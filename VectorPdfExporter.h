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
#include <QList>
#include <functional>

/**
 * Creates a hybrid "sandwich" PDF:
 *   - Layer 1: raster image (the processed scan)
 *   - Layer 2: invisible OCR text overlay (searchable/selectable)
 *
 * Requires Tesseract OCR (compile with -DENABLE_TESSERACT=ON).
 */
class VectorPdfExporter
{
public:
	struct Options {
		QString language;     ///< Tesseract language code (e.g. "eng", "ita")
		QString tessDataPath; ///< Path to tessdata dir (empty = default)
		int     dpi;          ///< Resolution for text placement (default 300)
		int     jpegQuality;  ///< JPEG quality 1-100 (default 65, good for scans)

		Options() : language("eng"), dpi(300), jpegQuality(65) {}
	};

	/// Progress callback: (currentPage, totalPages) -> should_cancel
	typedef std::function<bool(int, int)> ProgressCallback;

	/**
	 * Export all processed pages to a searchable PDF.
	 *
	 * @param pages          Project pages
	 * @param fileNameGen    Maps page IDs to output file paths
	 * @param outputPdfPath  Where to write the PDF
	 * @param opts           OCR options
	 * @param progress       Optional progress callback
	 * @return number of pages exported, or -1 on error
	 */
	static int exportSearchablePdf(
		IntrusivePtr<ProjectPages> const& pages,
		OutputFileNameGenerator const& fileNameGen,
		QString const& outputPdfPath,
		Options const& opts = Options(),
		ProgressCallback progress = ProgressCallback()
	);

	/**
	 * Standalone: vectorize any PDF or list of images.
	 * Renders each page/image, runs OCR, produces searchable PDF.
	 *
	 * @param inputImages   List of image file paths (TIFF, PNG, etc.)
	 * @param outputPdfPath Where to write the PDF
	 * @param opts          OCR options
	 * @param progress      Optional progress callback
	 * @return number of pages exported, or -1 on error
	 */
	static int vectorizeImages(
		QStringList const& inputImages,
		QString const& outputPdfPath,
		Options const& opts = Options(),
		ProgressCallback progress = ProgressCallback()
	);

	/**
	 * Standalone: vectorize an existing PDF file.
	 * Renders each PDF page to an image, runs OCR, produces searchable PDF.
	 *
	 * @param inputPdfPath  Path to input PDF
	 * @param outputPdfPath Where to write the searchable PDF
	 * @param opts          OCR options
	 * @param progress      Optional progress callback
	 * @return number of pages exported, or -1 on error
	 */
	static int vectorizePdf(
		QString const& inputPdfPath,
		QString const& outputPdfPath,
		Options const& opts = Options(),
		ProgressCallback progress = ProgressCallback()
	);

private:
	struct OcrWord {
		QString text;
		int x, y, w, h;   ///< Bounding box in image pixels
		float confidence;
	};

	/// Run OCR on a single image, return list of words with bounding boxes.
	static QList<OcrWord> ocrImage(
		QImage const& img,
		Options const& opts
	);

	/// Write compact PDF with JPEG image streams + invisible OCR text.
	static bool writeCompactPdf(
		QString const& outputPdfPath,
		QList<QImage> const& images,
		QList<QList<OcrWord>> const& allWords,
		Options const& opts
	);
};

#endif // VECTOR_PDF_EXPORTER_H_
