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

#include "MultiPageTiffExporter.h"
#include "ProjectPages.h"
#include "OutputFileNameGenerator.h"
#include "PageSequence.h"
#include "PageInfo.h"
#include "PageId.h"
#include "PageView.h"

#include <QImage>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QtGlobal>
#include <tiff.h>
#include <tiffio.h>

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static TIFF* openTiffForWrite(QString const& path)
{
#ifdef Q_OS_WIN
	return TIFFOpenW(
		reinterpret_cast<const wchar_t*>(path.utf16()), "w");
#else
	return TIFFOpen(path.toUtf8().constData(), "w");
#endif
}

static uint16_t toTiffCompression(MultiPageTiffExporter::Compression c)
{
	switch (c) {
		case MultiPageTiffExporter::COMPRESS_LZW:      return COMPRESSION_LZW;
		case MultiPageTiffExporter::COMPRESS_CCITT_G4:  return COMPRESSION_CCITTFAX4;
		case MultiPageTiffExporter::COMPRESS_ZIP:       return COMPRESSION_DEFLATE;
		case MultiPageTiffExporter::COMPRESS_NONE:      return COMPRESSION_NONE;
	}
	return COMPRESSION_LZW;
}

/// Determine correct PHOTOMETRIC for a 1-bit QImage by inspecting its color table.
static uint16_t monoPhotometric(QImage const& img)
{
	// QImage Format_Mono color table: index 0 and 1 map to colors.
	// MINISWHITE: 0=white, 1=black.  MINISBLACK: 0=black, 1=white.
	if (img.colorCount() >= 2) {
		int const lum0 = qGray(img.color(0));
		int const lum1 = qGray(img.color(1));
		if (lum0 > lum1) {
			return PHOTOMETRIC_MINISWHITE; // 0=white, 1=black
		}
	}
	return PHOTOMETRIC_MINISBLACK; // 0=black, 1=white
}

static bool writePage(TIFF* tif, QImage const& img, int dpi, uint16_t compression,
                      int pageIndex, int exportedTotal)
{
	int const w = img.width();
	int const h = img.height();

	if (w == 0 || h == 0) {
		return false;
	}

	bool const isBW = (img.format() == QImage::Format_Mono ||
	                   img.format() == QImage::Format_MonoLSB);
	bool const isGray = (img.format() == QImage::Format_Grayscale8 ||
	                    img.format() == QImage::Format_Indexed8);

	int bitsPerSample = 8;
	int samplesPerPixel = 1;
	uint16_t photometric = PHOTOMETRIC_MINISBLACK;

	if (isBW) {
		bitsPerSample = 1;
		samplesPerPixel = 1;
		photometric = monoPhotometric(img);
	} else if (isGray) {
		bitsPerSample = 8;
		samplesPerPixel = 1;
		photometric = PHOTOMETRIC_MINISBLACK;
		if (compression == COMPRESSION_CCITTFAX4) {
			compression = COMPRESSION_LZW;
		}
	} else {
		bitsPerSample = 8;
		samplesPerPixel = 3;
		photometric = PHOTOMETRIC_RGB;
		if (compression == COMPRESSION_CCITTFAX4) {
			compression = COMPRESSION_LZW;
		}
	}

	TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (uint32_t)w);
	TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (uint32_t)h);
	TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (uint16_t)bitsPerSample);
	TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)samplesPerPixel);
	TIFFSetField(tif, TIFFTAG_COMPRESSION, compression);
	TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, photometric);
	TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

	// G4 traditionally uses single strip; LZW/Deflate benefit from smaller strips
	if (compression == COMPRESSION_CCITTFAX4) {
		TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, (uint32_t)h);
	} else {
		uint32_t rows = TIFFDefaultStripSize(tif, 0);
		TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rows);
	}

	if (dpi > 0) {
		TIFFSetField(tif, TIFFTAG_XRESOLUTION, (float)dpi);
		TIFFSetField(tif, TIFFTAG_YRESOLUTION, (float)dpi);
		TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	}

	// Multi-page directory tags
	TIFFSetField(tif, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
	TIFFSetField(tif, TIFFTAG_PAGENUMBER, (uint16_t)pageIndex, (uint16_t)exportedTotal);

	if (isBW) {
		QImage mono = img;
		if (img.format() == QImage::Format_MonoLSB) {
			mono = img.convertToFormat(QImage::Format_Mono);
		}
		for (int y = 0; y < h; ++y) {
			if (TIFFWriteScanline(tif, const_cast<uchar*>(mono.constScanLine(y)), y) < 0) {
				return false;
			}
		}
	} else if (isGray) {
		QImage gray = img;
		if (img.format() == QImage::Format_Indexed8) {
			gray = img.convertToFormat(QImage::Format_Grayscale8);
		}
		for (int y = 0; y < h; ++y) {
			if (TIFFWriteScanline(tif, const_cast<uchar*>(gray.constScanLine(y)), y) < 0) {
				return false;
			}
		}
	} else {
		QImage rgb = img.convertToFormat(QImage::Format_RGB888);
		for (int y = 0; y < h; ++y) {
			if (TIFFWriteScanline(tif, const_cast<uchar*>(rgb.constScanLine(y)), y) < 0) {
				return false;
			}
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
// MultiPageTiffExporter::exportTiff
// ---------------------------------------------------------------------------

MultiPageTiffExporter::ExportResult MultiPageTiffExporter::exportTiff(
	IntrusivePtr<ProjectPages> const& pages,
	OutputFileNameGenerator const& fileNameGen,
	QString const& outputPath,
	Options const& opts,
	ProgressCallback progress)
{
	ExportResult result;
	result.pageCount = 0;
	result.fileSize = 0;

	PageSequence const seq = pages->toPageSequence(PAGE_VIEW);
	int const total = static_cast<int>(seq.numPages());

	if (total == 0) {
		result.errorDetail = QObject::tr("No pages in project.");
		result.pageCount = -1;
		return result;
	}

	// Pre-scan to count available pages for accurate PAGENUMBER tag
	int availablePages = 0;
	for (int i = 0; i < total; ++i) {
		QString const srcPath = fileNameGen.filePathFor(seq.pageAt(i).id());
		if (QFileInfo::exists(srcPath)) {
			++availablePages;
		}
	}

	TIFF* tif = openTiffForWrite(outputPath);
	if (!tif) {
		result.errorDetail = QObject::tr("Cannot create TIFF file: %1").arg(outputPath);
		result.pageCount = -1;
		return result;
	}

	uint16_t const tiffComp = toTiffCompression(opts.compression);
	int exported = 0;

	for (int i = 0; i < total; ++i) {
		if (progress && progress(i, total)) {
			TIFFClose(tif);
			QFile::remove(outputPath); // Clean up partial file
			result.errorDetail = QObject::tr("Export cancelled.");
			result.pageCount = -1;
			return result;
		}

		PageInfo const& info = seq.pageAt(i);
		QString const srcPath = fileNameGen.filePathFor(info.id());

		QImage img(srcPath);
		if (img.isNull() || img.width() == 0 || img.height() == 0) {
			continue;
		}

		// DPI resampling BEFORE binarization (so SmoothTransformation works on grayscale)
		if (opts.dpi > 0) {
			int srcDpi = qRound(img.dotsPerMeterX() * 0.0254);
			if (srcDpi > 0 && srcDpi != opts.dpi) {
				double scale = double(opts.dpi) / srcDpi;
				img = img.scaled(
					qRound(img.width() * scale),
					qRound(img.height() * scale),
					Qt::IgnoreAspectRatio,
					Qt::SmoothTransformation
				);
				img.setDotsPerMeterX(qRound(opts.dpi / 0.0254));
				img.setDotsPerMeterY(qRound(opts.dpi / 0.0254));
			}
		}

		// Force binarization AFTER scaling
		if (opts.forceBW && img.format() != QImage::Format_Mono &&
		    img.format() != QImage::Format_MonoLSB) {
			QImage gray = img.convertToFormat(QImage::Format_Grayscale8);
			QImage bw(gray.width(), gray.height(), QImage::Format_Mono);
			bw.setColor(0, qRgb(255, 255, 255)); // 0 = white
			bw.setColor(1, qRgb(0, 0, 0));       // 1 = black
			for (int y = 0; y < gray.height(); ++y) {
				uchar const* src = gray.constScanLine(y);
				for (int x = 0; x < gray.width(); ++x) {
					bw.setPixel(x, y, src[x] < 128 ? 1 : 0);
				}
			}
			if (opts.dpi > 0) {
				bw.setDotsPerMeterX(qRound(opts.dpi / 0.0254));
				bw.setDotsPerMeterY(qRound(opts.dpi / 0.0254));
			}
			img = bw;
		}

		if (!writePage(tif, img, opts.dpi, tiffComp, exported, availablePages)) {
			TIFFClose(tif);
			QFile::remove(outputPath); // Clean up partial file
			result.errorDetail = QObject::tr("Failed to write page %1.").arg(i + 1);
			result.pageCount = -1;
			return result;
		}

		TIFFWriteDirectory(tif);
		++exported;
	}

	TIFFClose(tif);

	if (exported == 0) {
		QFile::remove(outputPath); // Clean up empty file
		result.errorDetail = QObject::tr(
			"No processed output images found. "
			"Run batch processing before exporting.");
		result.pageCount = -1;
		return result;
	}

	result.pageCount = exported;
	result.fileSize = QFileInfo(outputPath).size();
	return result;
}
