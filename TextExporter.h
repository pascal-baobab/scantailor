/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TEXT_EXPORTER_H_
#define TEXT_EXPORTER_H_

#include "IntrusivePtr.h"
#include <QString>
#include <functional>

class ProjectPages;
class OutputFileNameGenerator;

/**
 * Exports OCR'd plain text from all processed pages using Tesseract.
 *
 * Produces either a single concatenated .txt file or individual per-page
 * text files. Useful for:
 *   - Full-text search indexing
 *   - Text mining and NLP pipelines
 *   - Accessibility (screen readers)
 *   - Content extraction without image overhead
 *
 * Output encoding is always UTF-8.
 */
class TextExporter
{
public:
	enum OutputMode {
		SINGLE_FILE,    ///< All pages concatenated into one .txt (default)
		PER_PAGE_FILES  ///< One .txt per page in a directory
	};

	struct Options {
		QString language;      ///< Tesseract language code (e.g. "eng", "ita+eng")
		QString tessDataPath;  ///< Parent dir containing tessdata/ (empty = default)
		OutputMode mode;       ///< Single file or per-page (default SINGLE_FILE)
		bool insertPageBreaks; ///< Insert form-feed (\\f) between pages (default true)
		bool insertPageHeaders;///< Insert "--- Page N ---" headers (default true)
		int  dpi;              ///< Hint DPI for Tesseract (default 300)

		Options()
			: language("eng"), mode(SINGLE_FILE),
			  insertPageBreaks(true), insertPageHeaders(true),
			  dpi(300) {}
	};

	struct ExportResult {
		int    pageCount;    ///< Pages with text (0 = none, -1 = error)
		int    wordCount;    ///< Total words extracted
		int    charCount;    ///< Total characters extracted
		bool   tessInitOk;   ///< Whether Tesseract initialized
		QString errorDetail; ///< Error message if something went wrong
	};

	/// Progress callback: (currentPage, totalPages) -> should_cancel
	typedef std::function<bool(int, int)> ProgressCallback;

	/**
	 * Export OCR text from all processed pages.
	 *
	 * \param pages        Project page collection.
	 * \param fileNameGen  Knows where output images are on disk.
	 * \param outputPath   Destination file (.txt) or directory (per-page mode).
	 * \param opts         Export options.
	 * \param progress     Optional progress callback.
	 * \return Export result with word/char counts.
	 */
	static ExportResult exportText(
		IntrusivePtr<ProjectPages> const& pages,
		OutputFileNameGenerator const& fileNameGen,
		QString const& outputPath,
		Options const& opts = Options(),
		ProgressCallback progress = ProgressCallback()
	);
};

#endif // TEXT_EXPORTER_H_
