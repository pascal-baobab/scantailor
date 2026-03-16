/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "VectorPdfExporter.h"
#include "PageSequence.h"
#include "PageInfo.h"
#include "PageView.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QTextCodec>
#include <QPair>
#include <algorithm>
#include <cstdio>

#ifdef HAVE_TESSERACT
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#endif

// ==================== Compact PDF Writer ====================
//
// Writes PDF directly with JPEG (DCTDecode) image streams.
// QPrinter uses lossless Flate which bloats scanned pages.
// JPEG at quality 65 gives ~5-10x smaller files for photographs/scans.

bool
VectorPdfExporter::writeCompactPdf(
	QString const& outputPdfPath,
	QList<QImage> const& images,
	QList<QList<OcrWord>> const& allWords,
	Options const& opts)
{
	QFile file(outputPdfPath);
	if (!file.open(QIODevice::WriteOnly)) {
		return false;
	}

	int nextObjId = 1;
	QList<QPair<int, qint64>> xrefEntries;

	// Helper: write a PDF object, return its ID.
	auto writeObj = [&](QByteArray const& content) -> int {
		int id = nextObjId++;
		xrefEntries.append(qMakePair(id, file.pos()));
		file.write(QByteArray::number(id) + " 0 obj\n");
		file.write(content);
		file.write("\nendobj\n");
		return id;
	};

	// PDF header.
	file.write("%PDF-1.4\n%\xE2\xE3\xCF\xD3\n");

	// Font object — Helvetica (standard PDF font, no embedding needed).
	int const fontId = writeObj(
		"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>"
	);

	// Reserve Pages object ID (written later, needs child IDs).
	int const pagesObjId = nextObjId++;
	QList<int> pageObjIds;

	for (int p = 0; p < images.size(); ++p) {
		QImage const& origImg = images[p];

		// --- Compress image to JPEG ---
		QImage rgbImg = origImg.convertToFormat(QImage::Format_RGB888);
		QByteArray jpegData;
		{
			QBuffer buffer(&jpegData);
			buffer.open(QIODevice::WriteOnly);
			rgbImg.save(&buffer, "JPEG", opts.jpegQuality);
		}

		int const imgW = rgbImg.width();
		int const imgH = rgbImg.height();

		// Image XObject with DCTDecode (= raw JPEG).
		QByteArray imgObj;
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

		int const imgObjId = writeObj(imgObj);

		// Page dimensions in PDF points (72 pt/inch).
		double const pageW = (origImg.width() / static_cast<double>(opts.dpi)) * 72.0;
		double const pageH = (origImg.height() / static_cast<double>(opts.dpi)) * 72.0;

		// --- Build content stream: draw image + invisible OCR text ---
		QByteArray content;

		// Draw image scaled to full page.
		content += "q\n";
		content += QByteArray::number(pageW, 'f', 2) + " 0 0 "
		         + QByteArray::number(pageH, 'f', 2) + " 0 0 cm\n";
		content += "/Img Do\n";
		content += "Q\n";

		// Invisible OCR text overlay.
		if (p < allWords.size() && !allWords[p].isEmpty()) {
			QList<OcrWord> const& words = allWords[p];
			double const origW = origImg.width();
			double const origH = origImg.height();

			content += "BT\n";
			content += "3 Tr\n"; // Rendering mode 3 = invisible

			for (int w = 0; w < words.size(); ++w) {
				OcrWord const& word = words[w];
				if (word.text.isEmpty()) {
					continue;
				}

				// Convert pixel coordinates to PDF points.
				double const x = (word.x / origW) * pageW;
				// PDF y-axis goes upward from bottom.
				double const y = pageH - ((word.y + word.h) / origH) * pageH;
				double fontSize = (word.h / origH) * pageH * 0.9;
				if (fontSize < 1.0) {
					fontSize = 1.0;
				}

				content += "/F1 " + QByteArray::number(fontSize, 'f', 1) + " Tf\n";
				content += QByteArray::number(x, 'f', 2) + " "
				         + QByteArray::number(y, 'f', 2) + " Td\n";

				// Encode text as UTF-16BE hex string for Unicode support.
				QByteArray hex = "<FEFF"; // BOM
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

		// Content stream object.
		QByteArray contentObj;
		contentObj += "<< /Length " + QByteArray::number(content.size()) + " >>\n";
		contentObj += "stream\n";
		contentObj += content;
		contentObj += "endstream";

		int const contentObjId = writeObj(contentObj);

		// Page object.
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

	// Pages object (reserved earlier).
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

	// Catalog.
	int const catalogId = writeObj(
		"<< /Type /Catalog /Pages " + QByteArray::number(pagesObjId) + " 0 R >>"
	);

	// Cross-reference table.
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

	// Trailer.
	file.write("trailer\n");
	file.write("<< /Size " + QByteArray::number(nextObjId)
	         + " /Root " + QByteArray::number(catalogId) + " 0 R >>\n");
	file.write("startxref\n");
	file.write(QByteArray::number(xrefOffset) + "\n");
	file.write("%%EOF\n");

	file.close();
	return true;
}

// ==================== Public Methods ====================

int
VectorPdfExporter::exportSearchablePdf(
	IntrusivePtr<ProjectPages> const& pages,
	OutputFileNameGenerator const& fileNameGen,
	QString const& outputPdfPath,
	Options const& opts,
	ProgressCallback progress)
{
	PageSequence const seq = pages->toPageSequence(PAGE_VIEW);
	int const totalPages = static_cast<int>(seq.numPages());

	if (totalPages == 0) {
		return 0;
	}

	QDir().mkpath(QFileInfo(outputPdfPath).absolutePath());

	QList<QImage> images;
	QList<QList<OcrWord>> allWords;

	for (int i = 0; i < totalPages; ++i) {
		if (progress && progress(i, totalPages)) {
			break; // cancelled
		}

		PageInfo const& info = seq.pageAt(i);
		QString const imgPath = fileNameGen.filePathFor(info.id());
		QImage img(imgPath);
		if (img.isNull()) {
			continue;
		}

		images.append(img);
		allWords.append(ocrImage(img, opts));
	}

	if (images.isEmpty()) {
		return 0;
	}

	if (!writeCompactPdf(outputPdfPath, images, allWords, opts)) {
		return -1;
	}

	if (progress) {
		progress(totalPages, totalPages);
	}

	return images.size();
}

// --------------- vectorizeImages ---------------

int
VectorPdfExporter::vectorizeImages(
	QStringList const& inputImages,
	QString const& outputPdfPath,
	Options const& opts,
	ProgressCallback progress)
{
	int const total = inputImages.size();
	if (total == 0) {
		return 0;
	}

	QDir().mkpath(QFileInfo(outputPdfPath).absolutePath());

	QList<QImage> images;
	QList<QList<OcrWord>> allWords;

	for (int i = 0; i < total; ++i) {
		if (progress && progress(i, total)) {
			break;
		}

		QImage img(inputImages[i]);
		if (img.isNull()) {
			continue;
		}

		images.append(img);
		allWords.append(ocrImage(img, opts));
	}

	if (images.isEmpty()) {
		return 0;
	}

	if (!writeCompactPdf(outputPdfPath, images, allWords, opts)) {
		return -1;
	}

	if (progress) {
		progress(total, total);
	}

	return images.size();
}

// --------------- vectorizePdf ---------------

#ifdef HAVE_POPPLER
#include <poppler-qt5.h>
#endif

int
VectorPdfExporter::vectorizePdf(
	QString const& inputPdfPath,
	QString const& outputPdfPath,
	Options const& opts,
	ProgressCallback progress)
{
#ifdef HAVE_POPPLER
	Poppler::Document* doc = Poppler::Document::load(inputPdfPath);
	if (!doc || doc->isLocked()) {
		delete doc;
		return -1;
	}

	doc->setRenderHint(Poppler::Document::Antialiasing, true);
	doc->setRenderHint(Poppler::Document::TextAntialiasing, true);

	int const total = doc->numPages();
	if (total == 0) {
		delete doc;
		return 0;
	}

	QList<QImage> images;
	QList<QList<OcrWord>> allWords;

	for (int i = 0; i < total; ++i) {
		if (progress && progress(i, total)) {
			break;
		}

		Poppler::Page* page = doc->page(i);
		if (!page) continue;

		QImage img = page->renderToImage(opts.dpi, opts.dpi);
		delete page;

		if (img.isNull()) continue;

		images.append(img);
		allWords.append(ocrImage(img, opts));
	}

	delete doc;

	if (images.isEmpty()) {
		return 0;
	}

	if (!writeCompactPdf(outputPdfPath, images, allWords, opts)) {
		return -1;
	}

	if (progress) {
		progress(total, total);
	}

	return images.size();
#else
	Q_UNUSED(inputPdfPath);
	Q_UNUSED(outputPdfPath);
	Q_UNUSED(opts);
	Q_UNUSED(progress);
	return -1; // Poppler not available
#endif
}

// ==================== OCR ====================

#ifdef HAVE_TESSERACT

QList<VectorPdfExporter::OcrWord>
VectorPdfExporter::ocrImage(QImage const& img, Options const& opts)
{
	QList<OcrWord> result;

	tesseract::TessBaseAPI api;

	QByteArray langBytes = opts.language.toUtf8();
	char const* tessDataDir = opts.tessDataPath.isEmpty()
		? nullptr
		: opts.tessDataPath.toUtf8().constData();

	if (api.Init(tessDataDir, langBytes.constData()) != 0) {
		return result;
	}

	QImage gray = img.convertToFormat(QImage::Format_Grayscale8);
	if (gray.isNull()) {
		gray = img.convertToFormat(QImage::Format_RGB888)
				   .convertToFormat(QImage::Format_Grayscale8);
	}

	api.SetImage(
		gray.constBits(),
		gray.width(),
		gray.height(),
		1,
		gray.bytesPerLine()
	);

	api.SetSourceResolution(opts.dpi);

	if (api.Recognize(nullptr) != 0) {
		api.End();
		return result;
	}

	tesseract::ResultIterator* ri = api.GetIterator();
	if (ri) {
		tesseract::PageIteratorLevel const level = tesseract::RIL_WORD;

		do {
			char const* text = ri->GetUTF8Text(level);
			if (!text) {
				continue;
			}

			OcrWord word;
			word.text = QString::fromUtf8(text);
			word.confidence = ri->Confidence(level);

			int x1, y1, x2, y2;
			ri->BoundingBox(level, &x1, &y1, &x2, &y2);
			word.x = x1;
			word.y = y1;
			word.w = x2 - x1;
			word.h = y2 - y1;

			delete[] text;

			if (word.confidence < 10.0f) {
				continue;
			}

			result.append(word);
		} while (ri->Next(level));
	}

	api.End();
	return result;
}

#else // !HAVE_TESSERACT

QList<VectorPdfExporter::OcrWord>
VectorPdfExporter::ocrImage(QImage const&, Options const&)
{
	return QList<OcrWord>();
}

#endif // HAVE_TESSERACT
