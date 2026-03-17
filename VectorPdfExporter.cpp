/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "VectorPdfExporter.h"
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
#include <QTextStream>
#include <algorithm>
#include <cstdio>

#ifdef HAVE_TESSERACT
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#endif

// ==================== PDF Writer ====================
//
// Grayscale JPEG raster + invisible OCR text overlay.

bool
VectorPdfExporter::writePdf(
	QString const& outputPdfPath,
	QList<PageData> const& pages,
	Options const& opts)
{
	QFile file(outputPdfPath);
	if (!file.open(QIODevice::WriteOnly)) {
		return false;
	}

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

	file.write("%PDF-1.4\n%\xE2\xE3\xCF\xD3\n");

	int const fontId = writeObj(
		"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>"
	);

	int const pagesObjId = nextObjId++;
	QList<int> pageObjIds;

	for (int p = 0; p < pages.size(); ++p) {
		PageData const& pd = pages[p];
		QImage const& origImg = pd.image;

		// Detect grayscale for ~3x better JPEG compression.
		QImage::Format const fmt = origImg.format();
		bool isGray = (fmt == QImage::Format_Grayscale8
		            || fmt == QImage::Format_Mono
		            || fmt == QImage::Format_MonoLSB
		            || fmt == QImage::Format_Indexed8);
		if (!isGray && (fmt == QImage::Format_RGB32 || fmt == QImage::Format_ARGB32)) {
			isGray = true;
			int const step = qMax(1, qMin(origImg.width(), origImg.height()) / 30);
			for (int y = 0; y < origImg.height() && isGray; y += step) {
				QRgb const* line = reinterpret_cast<QRgb const*>(origImg.constScanLine(y));
				for (int x = 0; x < origImg.width() && isGray; x += step) {
					QRgb const px = line[x];
					if (qAbs(qRed(px) - qGreen(px)) > 4 || qAbs(qGreen(px) - qBlue(px)) > 4)
						isGray = false;
				}
			}
		}

		// DPI from TIFF metadata.
		double const imgDpi = (origImg.dotsPerMeterX() > 0)
		    ? origImg.dotsPerMeterX() / 39.3701
		    : static_cast<double>(opts.dpi);

		// No downscaling — preserve original resolution for quality.
		// File size is controlled via JPEG quality only.

		// JPEG quality: use the user-configured value directly.
		int const quality = opts.jpegQuality;

		// Encode as JPEG.
		QByteArray jpegData;
		QByteArray imgObj;
		int imgW, imgH;

		if (isGray) {
			QImage grayImg = origImg.convertToFormat(QImage::Format_Grayscale8);
			imgW = grayImg.width();
			imgH = grayImg.height();
			QBuffer buffer(&jpegData);
			buffer.open(QIODevice::WriteOnly);
			grayImg.save(&buffer, "JPEG", quality);

			imgObj += "<< /Type /XObject /Subtype /Image";
			imgObj += " /Width " + QByteArray::number(imgW);
			imgObj += " /Height " + QByteArray::number(imgH);
			imgObj += " /ColorSpace /DeviceGray";
			imgObj += " /BitsPerComponent 8";
			imgObj += " /Filter /DCTDecode";
			imgObj += " /Length " + QByteArray::number(jpegData.size());
			imgObj += " >>\nstream\n";
			imgObj += jpegData;
			imgObj += "\nendstream";
		} else {
			QImage rgbImg = origImg.convertToFormat(QImage::Format_RGB888);
			imgW = rgbImg.width();
			imgH = rgbImg.height();
			QBuffer buffer(&jpegData);
			buffer.open(QIODevice::WriteOnly);
			rgbImg.save(&buffer, "JPEG", quality);

			imgObj += "<< /Type /XObject /Subtype /Image";
			imgObj += " /Width " + QByteArray::number(imgW);
			imgObj += " /Height " + QByteArray::number(imgH);
			imgObj += " /ColorSpace /DeviceRGB";
			imgObj += " /BitsPerComponent 8";
			imgObj += " /Filter /DCTDecode";
			imgObj += " /Length " + QByteArray::number(jpegData.size());
			imgObj += " >>\nstream\n";
			imgObj += jpegData;
			imgObj += "\nendstream";
		}

		int const imgObjId = writeObj(imgObj);

		// Page size in PDF points (72 pt/inch).
		double const pageW = (origImg.width()  / imgDpi) * 72.0;
		double const pageH = (origImg.height() / imgDpi) * 72.0;

		// Content stream.
		QByteArray content;

		// Draw raster image.
		content += "q\n";
		content += QByteArray::number(pageW, 'f', 2) + " 0 0 "
		         + QByteArray::number(pageH, 'f', 2) + " 0 0 cm\n";
		content += "/Img Do\n";
		content += "Q\n";

		// Invisible OCR text overlay.
		if (!pd.ocrWords.isEmpty()) {
			double const origW = origImg.width();
			double const origH = origImg.height();

			content += "BT\n";
			content += "3 Tr\n";

			for (int w = 0; w < pd.ocrWords.size(); ++w) {
				OcrWord const& word = pd.ocrWords[w];
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

		QByteArray pageObj;
		pageObj += "<< /Type /Page";
		pageObj += " /Parent " + QByteArray::number(pagesObjId) + " 0 R";
		pageObj += " /MediaBox [0 0 "
		         + QByteArray::number(pageW, 'f', 2) + " "
		         + QByteArray::number(pageH, 'f', 2) + "]";
		pageObj += " /Contents " + QByteArray::number(contentObjId) + " 0 R";
		pageObj += " /Resources <<"
		           " /XObject << /Img " + QByteArray::number(imgObjId) + " 0 R >>"
		           " /Font << /F1 " + QByteArray::number(fontId) + " 0 R >>"
		           " >>";
		pageObj += " >>";

		int const pageObjId = writeObj(pageObj);
		pageObjIds.append(pageObjId);
	}

	// Pages object.
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
	return true;
}

// ==================== Public: exportPdf ====================

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

	// Debug log file (temporary — remove after OCR is working).
	QFile logFile("C:/Users/Pass/Documents/ocr_debug.log");
	logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
	QTextStream log(&logFile);

	PageSequence const seq = pages->toPageSequence(PAGE_VIEW);
	int const totalPages = static_cast<int>(seq.numPages());

	if (totalPages == 0) {
		return result;
	}

	QDir().mkpath(QFileInfo(outputPdfPath).absolutePath());

	QList<PageData> allPages;

	// ── Init Tesseract ──
#ifdef HAVE_TESSERACT
	tesseract::TessBaseAPI tessApi;
	{
		QByteArray langBytes = opts.language.toUtf8();

		// Find the tessdata directory that contains the needed .traineddata files.
		// Tesseract 5.x Init(datapath, ...) looks for datapath/*.traineddata
		// (i.e. files directly inside the path, NOT in a tessdata/ subdirectory).
		QStringList candidates;  // each entry is an actual tessdata/ directory

		// 1) User-supplied path — normalize to the tessdata dir itself.
		QString userPath = opts.tessDataPath;
		if (!userPath.isEmpty()) {
			if (userPath.endsWith("/tessdata") || userPath.endsWith("\\tessdata")) {
				candidates << userPath;
			} else if (userPath.endsWith("/tessdata/") || userPath.endsWith("\\tessdata\\")) {
				userPath.chop(1);
				candidates << userPath;
			} else {
				// Assume it's the parent — append tessdata.
				candidates << (userPath + "/tessdata");
			}
		}

		// 2) Well-known system tessdata directories.
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

		// Pick the first candidate that has the primary language .traineddata file.
		QString firstLang = opts.language.split('+').first();
		QString chosenPath;
		for (QString const& c : candidates) {
			QString trainedFile = c + "/" + firstLang + ".traineddata";
			if (QFile::exists(trainedFile)) {
				chosenPath = c;
				break;
			}
		}

		QByteArray pathBytes = chosenPath.toUtf8();
		char const* tessDataDir = chosenPath.isEmpty()
			? nullptr : pathBytes.constData();

		log << "Candidates: " << candidates.join(" | ") << "\n";
		log << "chosenPath: " << (chosenPath.isEmpty() ? "(empty)" : chosenPath) << "\n";
		log << "tessDataDir: " << (tessDataDir ? tessDataDir : "(nullptr)") << "\n";
		log << "lang: " << langBytes << "\n";
		log.flush();

		if (tessApi.Init(tessDataDir, langBytes.constData()) == 0) {
			tessApi.SetVariable("tessedit_do_invert", "0");
			result.tessInitOk = true;
			log << "Tesseract Init: OK\n";
		} else {
			result.errorDetail = QString("Tesseract init failed.\n"
				"Language: %1\nData path: %2\n"
				"Check that tessdata files exist.")
				.arg(opts.language)
				.arg(chosenPath.isEmpty() ? "(auto)" : chosenPath);
			log << "Tesseract Init: FAILED - " << result.errorDetail << "\n";
		}
		log.flush();
	}
#else
	result.errorDetail = "Tesseract not compiled (HAVE_TESSERACT not defined)";
	qWarning() << result.errorDetail;
#endif

	for (int i = 0; i < totalPages; ++i) {
		if (progress && progress(i, totalPages)) {
			break;
		}

		PageInfo const& info = seq.pageAt(i);
		QString const imgPath = fileNameGen.filePathFor(info.id());
		QFile tiffFile(imgPath);
		if (!tiffFile.open(QIODevice::ReadOnly)) {
			continue;
		}
		QImage img = TiffReader::readImage(tiffFile);
		if (img.isNull()) {
			continue;
		}

		if (progress && progress(i, totalPages)) {
			break;
		}

		PageData pd;
		pd.image = img;

		// ── OCR ──
#ifdef HAVE_TESSERACT
		if (result.tessInitOk) {
			double const actualDpi = (img.dotsPerMeterX() > 0)
				? img.dotsPerMeterX() / 39.3701
				: static_cast<double>(opts.dpi);

			QImage gray = img.convertToFormat(QImage::Format_Grayscale8);
			int const useDpi = static_cast<int>(actualDpi);

			log << "OCR page " << i << ": " << gray.width() << "x" << gray.height()
			    << " dpi=" << useDpi
			    << " bytesPerLine=" << gray.bytesPerLine()
			    << " format=" << gray.format() << "\n";
			log.flush();

			tessApi.SetImage(gray.constBits(), gray.width(), gray.height(),
				1, gray.bytesPerLine());
			tessApi.SetSourceResolution(useDpi > 0 ? useDpi : opts.dpi);

			int recResult = tessApi.Recognize(nullptr);
			log << "  Recognize() returned " << recResult << "\n";
			log.flush();

			if (recResult == 0) {
				tesseract::ResultIterator* ri = tessApi.GetIterator();
				int rawWords = 0, keptWords = 0;
				if (ri) {
					tesseract::PageIteratorLevel const level = tesseract::RIL_WORD;
					do {
						char const* text = ri->GetUTF8Text(level);
						if (!text) continue;
						++rawWords;
						OcrWord word;
						word.text       = QString::fromUtf8(text);
						word.confidence = ri->Confidence(level);
						delete[] text;
						if (word.confidence < 10.0f || word.text.trimmed().isEmpty())
							continue;
						int x1, y1, x2, y2;
						ri->BoundingBox(level, &x1, &y1, &x2, &y2);
						word.x = x1;
						word.y = y1;
						word.w = x2 - x1;
						word.h = y2 - y1;
						pd.ocrWords.append(word);
						++keptWords;
					} while (ri->Next(level));
				}
				log << "  rawWords=" << rawWords
				    << " keptWords=" << keptWords << "\n";
				log.flush();
			}
			result.ocrWordCount += pd.ocrWords.size();
			tessApi.Clear();
		}
#endif

		allPages.append(pd);
	}

#ifdef HAVE_TESSERACT
	if (result.tessInitOk) tessApi.End();
#endif

	if (allPages.isEmpty()) {
		return result;
	}

	if (!writePdf(outputPdfPath, allPages, opts)) {
		result.pageCount = -1;
		result.errorDetail = "Failed to write PDF file: " + outputPdfPath;
		return result;
	}

	result.pageCount = allPages.size();

	QFileInfo fi(outputPdfPath);
	qDebug() << "PDF done:" << result.pageCount << "pages,"
	         << fi.size() / 1024 << "KB,"
	         << result.ocrWordCount << "OCR words";

	if (progress) {
		progress(totalPages, totalPages);
	}

	return result;
}
