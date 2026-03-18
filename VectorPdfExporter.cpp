/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "VectorPdfExporter.h"
#include "PotraceVectorizer.h"
#include "TiffReader.h"
#include "PageSequence.h"
#include "PageInfo.h"
#include "PageView.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QByteArray>
#include <QPair>
#include <QDebug>
#include <algorithm>
#include <cstdio>

#ifdef HAVE_TESSERACT
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#endif

// ==================== Streaming PDF Writer ====================
//
// Single-pass: each page is loaded, OCR'd, JPEG-encoded, and written
// to the PDF file immediately — only one page in memory at a time.

VectorPdfExporter::ExportResult
VectorPdfExporter::exportPdf(
	IntrusivePtr<ProjectPages> const& pages,
	OutputFileNameGenerator const& fileNameGen,
	QString const& outputPdfPath,
	Options const& opts,
	ProgressCallback progress)
{
	ExportResult result;
	result.pageCount = 0;
	result.ocrWordCount = 0;
	result.tessInitOk = false;

	PageSequence const seq = pages->toPageSequence(PAGE_VIEW);
	int const totalPages = static_cast<int>(seq.numPages());

	if (totalPages == 0) {
		return result;
	}

	QDir().mkpath(QFileInfo(outputPdfPath).absolutePath());

	QFile file(outputPdfPath);
	if (!file.open(QIODevice::WriteOnly)) {
		result.pageCount = -1;
		result.errorDetail = "Cannot open file: " + outputPdfPath;
		return result;
	}

	// ── PDF object bookkeeping ──
	int nextObjId = 1;
	QList<QPair<int, qint64>> xrefEntries;

	auto writeObj = [&](QByteArray const& content) -> int {
		int id = nextObjId++;
		xrefEntries.append(qMakePair(id, file.pos()));
		file.write(QByteArray::number(id) + " 0 obj\n");
		file.write(content);
		file.write("\nendobj\n");
		return id;
	};

	// ── PDF header ──
	file.write("%PDF-1.4\n%\xE2\xE3\xCF\xD3\n");

	int const fontId = writeObj(
		"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>"
	);

	// Reserve the Pages object ID (written at the end).
	int const pagesObjId = nextObjId++;
	QList<int> pageObjIds;

	// ── Init Tesseract (once) ──
#ifdef HAVE_TESSERACT
	tesseract::TessBaseAPI tessApi;
	{
		QByteArray langBytes = opts.language.toUtf8();

		QStringList candidates;
		QString userPath = opts.tessDataPath;
		if (!userPath.isEmpty()) {
			if (userPath.endsWith("/tessdata") || userPath.endsWith("\\tessdata")) {
				candidates << userPath;
			} else if (userPath.endsWith("/tessdata/") || userPath.endsWith("\\tessdata\\")) {
				userPath.chop(1);
				candidates << userPath;
			} else {
				candidates << (userPath + "/tessdata");
			}
		}

		static QStringList const systemTessdataDirs = {
			"C:/msys64/mingw64/share/tessdata",
			"/usr/share/tessdata",
			"/usr/local/share/tessdata"
		};
		for (QString const& td : systemTessdataDirs) {
			if (QDir(td).exists()) {
				candidates << td;
			}
		}

		QString firstLang = opts.language.split('+').first();
		QString chosenPath;
		for (QString const& c : candidates) {
			if (QFile::exists(c + "/" + firstLang + ".traineddata")) {
				chosenPath = c;
				break;
			}
		}

		QByteArray pathBytes = chosenPath.toUtf8();
		char const* tessDataDir = chosenPath.isEmpty()
			? nullptr : pathBytes.constData();

		if (tessApi.Init(tessDataDir, langBytes.constData()) == 0) {
			tessApi.SetVariable("tessedit_do_invert", "0");
			result.tessInitOk = true;
		} else {
			result.errorDetail = QString("Tesseract init failed.\n"
				"Language: %1\nData path: %2")
				.arg(opts.language)
				.arg(chosenPath.isEmpty() ? "(auto)" : chosenPath);
		}
	}
#else
	result.errorDetail = "Tesseract not compiled (HAVE_TESSERACT not defined)";
#endif

	// ── Stream pages one at a time ──
	bool cancelled = false;
	for (int i = 0; i < totalPages; ++i) {
		// Progress: starting page i
		if (progress && progress(i, totalPages)) {
			cancelled = true;
			break;
		}

		PageInfo const& info = seq.pageAt(i);
		QString const imgPath = fileNameGen.filePathFor(info.id());
		QFile tiffFile(imgPath);
		if (!tiffFile.open(QIODevice::ReadOnly)) {
			continue;
		}
		QImage img = TiffReader::readImage(tiffFile);
		tiffFile.close();
		if (img.isNull()) {
			continue;
		}

		// Check cancel after loading
		if (progress && progress(i, totalPages)) {
			cancelled = true;
			break;
		}

		// ── OCR this page ──
		QList<OcrWord> ocrWords;
#ifdef HAVE_TESSERACT
		if (result.tessInitOk) {
			double const actualDpi = (img.dotsPerMeterX() > 0)
				? img.dotsPerMeterX() / 39.3701
				: static_cast<double>(opts.dpi);

			QImage gray = img.convertToFormat(QImage::Format_Grayscale8);

			// Downscale to ~300 DPI for OCR — Tesseract quality plateaus
			// at 300 DPI, so processing 600 DPI wastes ~3x the time.
			double ocrScale = 1.0;
			int ocrDpi = static_cast<int>(actualDpi);
			if (ocrDpi > 350) {
				ocrScale = 300.0 / ocrDpi;
				gray = gray.scaled(
					static_cast<int>(gray.width() * ocrScale),
					static_cast<int>(gray.height() * ocrScale),
					Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				ocrDpi = 300;
			}

			tessApi.SetImage(gray.constBits(), gray.width(), gray.height(),
				1, gray.bytesPerLine());
			tessApi.SetSourceResolution(ocrDpi > 0 ? ocrDpi : opts.dpi);

			if (tessApi.Recognize(nullptr) == 0) {
				tesseract::ResultIterator* ri = tessApi.GetIterator();
				if (ri) {
					tesseract::PageIteratorLevel const level = tesseract::RIL_WORD;
					do {
						char const* text = ri->GetUTF8Text(level);
						if (!text) continue;
						OcrWord word;
						word.text       = QString::fromUtf8(text);
						word.confidence = ri->Confidence(level);
						delete[] text;
						if (word.confidence < 10.0f || word.text.trimmed().isEmpty())
							continue;
						int x1, y1, x2, y2;
						ri->BoundingBox(level, &x1, &y1, &x2, &y2);
						// Map OCR coordinates back to full-resolution image space.
						if (ocrScale < 1.0) {
							double const inv = 1.0 / ocrScale;
							x1 = static_cast<int>(x1 * inv);
							y1 = static_cast<int>(y1 * inv);
							x2 = static_cast<int>(x2 * inv);
							y2 = static_cast<int>(y2 * inv);
						}
						word.x = x1;
						word.y = y1;
						word.w = x2 - x1;
						word.h = y2 - y1;
						ocrWords.append(word);
					} while (ri->Next(level));
				}
				result.ocrWordCount += ocrWords.size();
			}
			tessApi.Clear();
		}
#endif

		// Check cancel after OCR
		if (progress && progress(i, totalPages)) {
			cancelled = true;
			break;
		}

		// ── Detect grayscale vs color ──
		QImage::Format const fmt = img.format();
		bool isGray = (fmt == QImage::Format_Grayscale8
		            || fmt == QImage::Format_Mono
		            || fmt == QImage::Format_MonoLSB
		            || fmt == QImage::Format_Indexed8);
		if (!isGray && (fmt == QImage::Format_RGB32 || fmt == QImage::Format_ARGB32)) {
			isGray = true;
			int const step = qMax(1, qMin(img.width(), img.height()) / 30);
			for (int y = 0; y < img.height() && isGray; y += step) {
				QRgb const* line = reinterpret_cast<QRgb const*>(img.constScanLine(y));
				for (int x = 0; x < img.width() && isGray; x += step) {
					QRgb const px = line[x];
					if (qAbs(qRed(px) - qGreen(px)) > 4 || qAbs(qGreen(px) - qBlue(px)) > 4)
						isGray = false;
				}
			}
		}

		double const imgDpi = (img.dotsPerMeterX() > 0)
		    ? img.dotsPerMeterX() / 39.3701
		    : static_cast<double>(opts.dpi);

		int const imgW = img.width();
		int const imgH = img.height();

		// ── Check for foreground TIFF (Mixed mode split output) ──
		// If the output was generated in Mixed mode with Split Output,
		// a separate *_foreground.tiff exists with pure B&W text.
		QString fgPath;
		QImage fgImg;
		if (opts.vectorize) {
			QFileInfo fi(imgPath);
			fgPath = fi.absolutePath() + "/" + fi.completeBaseName()
			       + "_foreground.tif";
			if (!QFile::exists(fgPath)) {
				fgPath = fi.absolutePath() + "/" + fi.completeBaseName()
				       + "_foreground.tiff";
			}
			if (QFile::exists(fgPath)) {
				QFile fgFile(fgPath);
				if (fgFile.open(QIODevice::ReadOnly)) {
					fgImg = TiffReader::readImage(fgFile);
					fgFile.close();
				}
			}
		}

		// Determine rendering mode for this page:
		//   hybrid  = foreground TIFF exists (JPEG bg + Potrace fg)
		//   vector  = B&W/gray page with vectorize enabled (Potrace only)
		//   raster  = color page or vectorize disabled (JPEG only)
		bool const useHybrid = opts.vectorize && !fgImg.isNull();
		bool const useVector = opts.vectorize && isGray && !useHybrid;

		// ── Try Potrace vectorization ──
		QByteArray potracePaths;
		double const pageW = (imgW / imgDpi) * 72.0;
		double const pageH = (imgH / imgDpi) * 72.0;

		if (useVector) {
			// Full-page vectorization for B&W/grayscale pages.
			potracePaths = PotraceVectorizer::traceToPathStream(
				img, pageW, pageH, static_cast<int>(imgDpi));
		} else if (useHybrid) {
			// Vectorize only the foreground (text) layer.
			potracePaths = PotraceVectorizer::traceToPathStream(
				fgImg, pageW, pageH, static_cast<int>(imgDpi));
		}
		fgImg = QImage(); // Release foreground image.

		// If Potrace returned empty (not compiled or tracing failed),
		// fall back to JPEG raster for this page.
		bool const hasVectors = !potracePaths.isEmpty();

		// ── Encode JPEG raster (when needed) ──
		int imgObjId = -1;
		bool needsRaster = !hasVectors || useHybrid;

		if (needsRaster) {
			QByteArray jpegData;
			if (isGray && !useHybrid) {
				QImage grayImg = img.convertToFormat(QImage::Format_Grayscale8);
				QBuffer buffer(&jpegData);
				buffer.open(QIODevice::WriteOnly);
				grayImg.save(&buffer, "JPEG", opts.jpegQuality);
			} else {
				QImage rgbImg = img.convertToFormat(QImage::Format_RGB888);
				QBuffer buffer(&jpegData);
				buffer.open(QIODevice::WriteOnly);
				rgbImg.save(&buffer, "JPEG", opts.jpegQuality);
			}

			QByteArray imgObj;
			imgObj += "<< /Type /XObject /Subtype /Image";
			imgObj += " /Width " + QByteArray::number(imgW);
			imgObj += " /Height " + QByteArray::number(imgH);
			imgObj += (isGray && !useHybrid)
			        ? " /ColorSpace /DeviceGray" : " /ColorSpace /DeviceRGB";
			imgObj += " /BitsPerComponent 8";
			imgObj += " /Filter /DCTDecode";
			imgObj += " /Length " + QByteArray::number(jpegData.size());
			imgObj += " >>\nstream\n";
			imgObj += jpegData;
			imgObj += "\nendstream";

			imgObjId = writeObj(imgObj);
		}

		// Release the original image.
		img = QImage();

		// ── Content stream ──
		QByteArray content;

		// Layer 1: JPEG raster background (for raster-only and hybrid pages).
		if (needsRaster && imgObjId >= 0) {
			content += "q\n";
			content += QByteArray::number(pageW, 'f', 2) + " 0 0 "
			         + QByteArray::number(pageH, 'f', 2) + " 0 0 cm\n";
			content += "/Img Do\n";
			content += "Q\n";
		}

		// Layer 2: Potrace vector paths (for vector-only and hybrid pages).
		if (hasVectors) {
			content += potracePaths;
		}
		potracePaths.clear();

		// Layer 3: Invisible OCR text overlay.
		if (!ocrWords.isEmpty()) {
			double const origW = imgW;
			double const origH = imgH;

			content += "BT\n";
			content += "3 Tr\n";

			for (int w = 0; w < ocrWords.size(); ++w) {
				OcrWord const& word = ocrWords[w];
				if (word.text.isEmpty()) continue;

				double const x = (word.x / origW) * pageW;
				double const y = pageH - ((word.y + word.h) / origH) * pageH;
				double fontSize = (word.h / origH) * pageH * 0.9;
				if (fontSize < 1.0) fontSize = 1.0;

				content += "/F1 " + QByteArray::number(fontSize, 'f', 1) + " Tf\n";
				content += "1 0 0 1 "
				         + QByteArray::number(x, 'f', 2) + " "
				         + QByteArray::number(y, 'f', 2) + " Tm\n";

				QByteArray hex = "<FEFF";
				QString const& txt = word.text;
				for (int c = 0; c < txt.size(); ++c) {
					ushort code = txt[c].unicode();
					char buf[5];
					std::snprintf(buf, sizeof(buf), "%04X", code);
					hex += buf;
				}
				hex += ">";
				content += hex + " Tj\n";
			}
			content += "ET\n";
		}

		QByteArray contentObj;
		contentObj += "<< /Length " + QByteArray::number(content.size()) + " >>\n";
		contentObj += "stream\n";
		contentObj += content;
		contentObj += "\nendstream";

		int const contentObjId = writeObj(contentObj);

		// ── Page object ──
		QByteArray pageObj;
		pageObj += "<< /Type /Page";
		pageObj += " /Parent " + QByteArray::number(pagesObjId) + " 0 R";
		pageObj += " /MediaBox [0 0 "
		         + QByteArray::number(pageW, 'f', 2) + " "
		         + QByteArray::number(pageH, 'f', 2) + "]";
		pageObj += " /Contents " + QByteArray::number(contentObjId) + " 0 R";
		pageObj += " /Resources <<";
		if (imgObjId >= 0) {
			pageObj += " /XObject << /Img " + QByteArray::number(imgObjId) + " 0 R >>";
		}
		pageObj += " /Font << /F1 " + QByteArray::number(fontId) + " 0 R >>"
		           " >>";
		pageObj += " >>";

		int const pageObjId = writeObj(pageObj);
		pageObjIds.append(pageObjId);
		++result.pageCount;
	}

#ifdef HAVE_TESSERACT
	if (result.tessInitOk) tessApi.End();
#endif

	if (cancelled || pageObjIds.isEmpty()) {
		file.close();
		if (cancelled) {
			// Clean up partial file on cancel.
			QFile::remove(outputPdfPath);
			result.pageCount = 0;
			result.errorDetail = "Cancelled by user";
		}
		return result;
	}

	// ── Pages catalog (deferred) ──
	xrefEntries.append(qMakePair(pagesObjId, file.pos()));
	file.write(QByteArray::number(pagesObjId) + " 0 obj\n");
	QByteArray pagesObj;
	pagesObj += "<< /Type /Pages /Kids [";
	for (int i = 0; i < pageObjIds.size(); ++i) {
		if (i > 0) pagesObj += " ";
		pagesObj += QByteArray::number(pageObjIds[i]) + " 0 R";
	}
	pagesObj += "] /Count " + QByteArray::number(pageObjIds.size()) + " >>";
	file.write(pagesObj);
	file.write("\nendobj\n");

	int const catalogId = writeObj(
		"<< /Type /Catalog /Pages " + QByteArray::number(pagesObjId) + " 0 R >>"
	);

	// ── Cross-reference table ──
	qint64 const xrefOffset = file.pos();

	std::sort(xrefEntries.begin(), xrefEntries.end(),
		[](QPair<int,qint64> const& a, QPair<int,qint64> const& b) {
			return a.first < b.first;
		}
	);

	file.write("xref\n");
	file.write("0 " + QByteArray::number(nextObjId) + "\n");
	file.write("0000000000 65535 f \n");
	for (int i = 0; i < xrefEntries.size(); ++i) {
		char buf[21];
		std::snprintf(buf, sizeof(buf), "%010lld 00000 n \n",
			static_cast<long long>(xrefEntries[i].second));
		file.write(buf, 20);
	}

	file.write("trailer\n");
	file.write("<< /Size " + QByteArray::number(nextObjId)
	         + " /Root " + QByteArray::number(catalogId) + " 0 R >>\n");
	file.write("startxref\n");
	file.write(QByteArray::number(xrefOffset) + "\n");
	file.write("%%EOF\n");

	file.close();

	if (progress) {
		progress(totalPages, totalPages);
	}

	return result;
}
