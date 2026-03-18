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

#ifndef JPEG_BATCH_EXPORTER_H_
#define JPEG_BATCH_EXPORTER_H_

#include "IntrusivePtr.h"
#include <QString>
#include <functional>

class ProjectPages;
class OutputFileNameGenerator;

/**
 * Exports all processed pages as individual JPEG files in a directory.
 *
 * JPEG batch export is useful for:
 *   - Web publishing (small files, universal browser support)
 *   - E-book creation pipelines (Calibre, Sigil)
 *   - Sharing via email or cloud storage
 *   - Mobile viewing (lightweight, fast loading)
 *
 * Files are named sequentially: page_001.jpg, page_002.jpg, ...
 * An optional index.html viewer is generated for quick browser preview.
 */
class JpegBatchExporter
{
public:
	struct Options {
		int  quality;          ///< JPEG quality 1-100 (default 85)
		int  dpi;              ///< Output DPI (0 = keep original)
		bool grayscale;        ///< Convert to grayscale (default false)
		bool generateViewer;   ///< Generate index.html viewer (default true)
		int  maxWidth;         ///< Max pixel width (0 = no limit)
		int  maxHeight;        ///< Max pixel height (0 = no limit)

		Options()
			: quality(85), dpi(0), grayscale(false),
			  generateViewer(true), maxWidth(0), maxHeight(0) {}
	};

	struct ExportResult {
		int    pageCount;    ///< Pages exported (0 = none found, -1 = error)
		qint64 totalSize;    ///< Total size of all JPEGs in bytes
		QString errorDetail; ///< Error message if something went wrong
	};

	/// Progress callback: (currentPage, totalPages) -> should_cancel
	typedef std::function<bool(int, int)> ProgressCallback;

	/**
	 * Export all processed pages as individual JPEG files.
	 *
	 * \param pages        Project page collection.
	 * \param fileNameGen  Knows where output images are on disk.
	 * \param outputDir    Destination directory (created if absent).
	 * \param opts         Export options.
	 * \param progress     Optional progress callback.
	 * \return Export result with page count and total size.
	 */
	static ExportResult exportJpegs(
		IntrusivePtr<ProjectPages> const& pages,
		OutputFileNameGenerator const& fileNameGen,
		QString const& outputDir,
		Options const& opts = Options(),
		ProgressCallback progress = ProgressCallback()
	);
};

#endif // JPEG_BATCH_EXPORTER_H_
