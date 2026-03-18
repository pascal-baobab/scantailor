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

#ifndef MULTI_PAGE_TIFF_EXPORTER_H_
#define MULTI_PAGE_TIFF_EXPORTER_H_

#include "IntrusivePtr.h"
#include <QString>
#include <functional>

class ProjectPages;
class OutputFileNameGenerator;

/**
 * Exports all processed ScanTailor pages into a single multi-page TIFF file.
 *
 * Multi-page TIFF is the standard archival format for scanned documents,
 * supported by all document management systems and OCR tools. Each page
 * becomes a separate TIFF directory (IFD) within one file.
 *
 * Compression options:
 *   - LZW: lossless, good for grayscale/color (~40-60% reduction)
 *   - CCITT G4: 1-bit fax compression (~90-95% reduction, B&W only)
 *   - ZIP/Deflate: lossless, good general-purpose (~50-70% reduction)
 *   - None: uncompressed (largest, fastest)
 */
class MultiPageTiffExporter
{
public:
	enum Compression {
		COMPRESS_LZW,       ///< Lossless LZW (default, good for mixed content)
		COMPRESS_CCITT_G4,  ///< CCITT Group 4 fax (B&W only, smallest)
		COMPRESS_ZIP,       ///< Deflate/ZIP (lossless)
		COMPRESS_NONE       ///< No compression
	};

	struct Options {
		Compression compression;  ///< Compression algorithm
		int  dpi;                 ///< Output DPI (0 = keep original)
		bool forceBW;             ///< Force binarization to 1-bit B&W

		Options() : compression(COMPRESS_LZW), dpi(0), forceBW(false) {}
	};

	struct ExportResult {
		int    pageCount;    ///< Pages exported (0 = none found, -1 = error)
		qint64 fileSize;     ///< Output file size in bytes
		QString errorDetail; ///< Error message if something went wrong
	};

	/// Progress callback: (currentPage, totalPages) -> should_cancel
	typedef std::function<bool(int, int)> ProgressCallback;

	/**
	 * Export all processed pages into a single multi-page TIFF.
	 *
	 * \param pages        Project page collection.
	 * \param fileNameGen  Knows where output images are on disk.
	 * \param outputPath   Destination .tif file path.
	 * \param opts         Export options.
	 * \param progress     Optional progress callback.
	 * \return Export result with page count and file size.
	 */
	static ExportResult exportTiff(
		IntrusivePtr<ProjectPages> const& pages,
		OutputFileNameGenerator const& fileNameGen,
		QString const& outputPath,
		Options const& opts = Options(),
		ProgressCallback progress = ProgressCallback()
	);
};

#endif // MULTI_PAGE_TIFF_EXPORTER_H_
