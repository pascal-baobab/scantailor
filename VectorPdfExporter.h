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
 *   - B&W/grayscale pages: Potrace vector paths (crisp at any zoom)
 *   - Color pages: JPEG raster (DCTDecode)
 *   - Hybrid pages (Mixed mode): JPEG background + Potrace foreground
 *   - Invisible OCR text overlay (searchable/selectable via Tesseract)
 */
class VectorPdfExporter
{
public:
	enum PageFormat { PAGE_AUTO, PAGE_A4, PAGE_A5, PAGE_LETTER };

	enum CompressionMode {
		COMPRESS_TEXT_OPTIMAL,  ///< Binarize → 1-bit FlateDecode (~40-70 KB/page)
		COMPRESS_JPEG,          ///< Grayscale/RGB JPEG (~150-500 KB/page)
		COMPRESS_LOSSLESS,      ///< 8-bit grayscale FlateDecode (~300-800 KB/page)
		COMPRESS_NONE           ///< Uncompressed (debug only)
	};

	struct Options {
		QString language;        ///< Tesseract language code (e.g. "eng", "eng+ita")
		QString tessDataPath;    ///< Parent dir containing tessdata/ (empty = default)
		int     dpi;             ///< Image resolution (default 300)
		int     jpegQuality;     ///< JPEG quality 1-100 (default 85)
		bool    vectorize;       ///< Use Potrace for 1-bit (default true)
		bool    enableOcr;       ///< OCR searchable text (default true)
		PageFormat pageFormat;   ///< Page size (default AUTO)
		CompressionMode compression; ///< Compression mode (default TEXT_OPTIMAL)
		bool    downsample;      ///< Downsample images above threshold (default false)
		int     downsampleThreshold; ///< Only downsample above this DPI (default 600)
		int     sharpening;      ///< 0=smooth/denoise, 50=neutral, 100=sharpen (default 30)
		QString pdfVersion;      ///< "1.4", "1.5", "1.7" (default "1.4")
		bool    forceGrayscale;  ///< Convert color to grayscale (default true)

		Options()
			: language("eng"), dpi(300), jpegQuality(85),
			  vectorize(true), enableOcr(true), pageFormat(PAGE_AUTO),
			  compression(COMPRESS_TEXT_OPTIMAL), downsample(false),
			  downsampleThreshold(600), sharpening(30),
			  pdfVersion("1.4"), forceGrayscale(true) {}
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
	 * Streaming single-pass: each page is loaded, OCR'd, JPEG-encoded,
	 * and written to the file immediately — only one page in memory at a time.
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
};

#endif // VECTOR_PDF_EXPORTER_H_
