/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#ifndef VECTOR_PDF_EXPORTER_H_
#define VECTOR_PDF_EXPORTER_H_

#include "OutputFileNameGenerator.h"
#include "ProjectPages.h"
#include "IntrusivePtr.h"
#include <QString>
#include <QStringList>
#include <QImage>
#include <QByteArray>
#include <QList>
#include <functional>

/**
 * Creates a compact searchable PDF from processed TIFF pages:
 *   - Grayscale JPEG raster (DCTDecode) for compact file size
 *   - Invisible OCR text overlay (searchable/selectable via Tesseract)
 */
class VectorPdfExporter
{
public:
	struct Options {
		QString language;     ///< Tesseract language code (e.g. "eng", "eng+ita")
		QString tessDataPath; ///< Parent dir containing tessdata/ (empty = default)
		int     dpi;          ///< Resolution (default 300)
		int     jpegQuality;  ///< JPEG quality 1-100

		Options() : language("eng"), dpi(300), jpegQuality(70) {}
	};

	/// Result from export with diagnostic info.
	struct ExportResult {
		int pageCount;       ///< Number of pages exported (0 = no pages, -1 = error)
		int ocrWordCount;    ///< Total OCR words found across all pages
		bool tessInitOk;     ///< Whether Tesseract initialized successfully
		QString errorDetail; ///< Error message if something went wrong
	};

	/// Progress callback: (currentPage, totalPages) -> should_cancel
	typedef std::function<bool(int, int)> ProgressCallback;

	/**
	 * Export all processed pages to a compact searchable PDF.
	 * Pipeline: load TIFFs → JPEG compress → Tesseract OCR → write PDF.
	 */
	static ExportResult exportPdf(
		IntrusivePtr<ProjectPages> const& pages,
		OutputFileNameGenerator const& fileNameGen,
		QString const& outputPdfPath,
		Options const& opts,
		ProgressCallback progress = ProgressCallback()
	);

private:
	struct OcrWord {
		QString text;
		int x, y, w, h;
		float confidence;
	};

	struct PageData {
		QImage image;
		QList<OcrWord> ocrWords;
	};

	static bool writePdf(
		QString const& outputPdfPath,
		QList<PageData> const& pages,
		Options const& opts
	);
};

#endif // VECTOR_PDF_EXPORTER_H_
