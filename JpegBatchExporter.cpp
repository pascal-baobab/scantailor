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

#include "JpegBatchExporter.h"
#include "ProjectPages.h"
#include "OutputFileNameGenerator.h"
#include "PageSequence.h"
#include "PageInfo.h"
#include "PageId.h"
#include "PageView.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QObject>
#include <QTextStream>

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static void generateHtmlViewer(
	QString const& outputDir, int pageCount, QString const& prefix)
{
	QString const htmlPath = QDir(outputDir).filePath(
		QLatin1String("index.html"));
	QFile file(htmlPath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return; // Non-fatal: viewer is optional
	}

	QTextStream out(&file);
	out.setCodec("UTF-8");
	out << QLatin1String("<!DOCTYPE html>\n<html>\n<head>\n");
	out << QLatin1String("<meta charset=\"utf-8\">\n");
	out << QLatin1String("<title>ScanTailor Export</title>\n");
	out << QLatin1String("<style>\n");
	out << QLatin1String("body { background: #333; margin: 0; padding: 20px; "
	                     "text-align: center; font-family: sans-serif; }\n");
	out << QLatin1String("h1 { color: #eee; }\n");
	out << QLatin1String("img { max-width: 100%; margin: 10px auto; "
	                     "display: block; box-shadow: 0 2px 8px rgba(0,0,0,0.5); }\n");
	out << QLatin1String(".page-num { color: #aaa; font-size: 14px; }\n");
	out << QLatin1String("</style>\n</head>\n<body>\n");
	out << QString::fromLatin1("<h1>ScanTailor Export &mdash; %1 pages</h1>\n")
	       .arg(pageCount);

	for (int i = 0; i < pageCount; ++i) {
		QString const fname = QString::fromLatin1("%1%2.jpg")
			.arg(prefix)
			.arg(i + 1, 3, 10, QLatin1Char('0'));
		out << QString::fromLatin1(
			"<div class=\"page-num\">Page %1</div>\n"
			"<img src=\"%2\" alt=\"Page %1\">\n")
			.arg(i + 1).arg(fname);
	}

	out << QLatin1String("</body>\n</html>\n");
	file.close();
}

// ---------------------------------------------------------------------------
// JpegBatchExporter::exportJpegs
// ---------------------------------------------------------------------------

JpegBatchExporter::ExportResult JpegBatchExporter::exportJpegs(
	IntrusivePtr<ProjectPages> const& pages,
	OutputFileNameGenerator const& fileNameGen,
	QString const& outputDir,
	Options const& opts,
	ProgressCallback progress)
{
	ExportResult result;
	result.pageCount = 0;
	result.totalSize = 0;

	QDir dir(outputDir);
	if (!dir.mkpath(QLatin1String("."))) {
		result.errorDetail = QObject::tr("Cannot create output directory: %1")
			.arg(outputDir);
		result.pageCount = -1;
		return result;
	}

	PageSequence const seq = pages->toPageSequence(PAGE_VIEW);
	int const total = static_cast<int>(seq.numPages());

	if (total == 0) {
		result.errorDetail = QObject::tr("No pages in project.");
		result.pageCount = -1;
		return result;
	}

	QString const prefix = QLatin1String("page_");
	int exported = 0;

	for (int i = 0; i < total; ++i) {
		if (progress && progress(i, total)) {
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

		// DPI resampling FIRST (before maxWidth/maxHeight capping)
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
			}
			// Set correct DPI metadata on the output image
			img.setDotsPerMeterX(qRound(opts.dpi / 0.0254));
			img.setDotsPerMeterY(qRound(opts.dpi / 0.0254));
		}

		// Convert to grayscale if requested
		if (opts.grayscale) {
			img = img.convertToFormat(QImage::Format_Grayscale8);
		}

		// Resize if max dimensions specified (after DPI scaling)
		if (opts.maxWidth > 0 || opts.maxHeight > 0) {
			int maxW = opts.maxWidth > 0 ? opts.maxWidth : img.width();
			int maxH = opts.maxHeight > 0 ? opts.maxHeight : img.height();
			if (img.width() > maxW || img.height() > maxH) {
				img = img.scaled(maxW, maxH,
					Qt::KeepAspectRatio, Qt::SmoothTransformation);
			}
		}

		// Ensure compatible format for JPEG saving (strip alpha, handle mono)
		switch (img.format()) {
			case QImage::Format_Mono:
			case QImage::Format_MonoLSB:
			case QImage::Format_Indexed8:
				img = opts.grayscale
					? img.convertToFormat(QImage::Format_Grayscale8)
					: img.convertToFormat(QImage::Format_RGB888);
				break;
			case QImage::Format_ARGB32:
			case QImage::Format_ARGB32_Premultiplied:
			case QImage::Format_RGBA8888:
			case QImage::Format_RGBA8888_Premultiplied:
				if (!opts.grayscale) {
					img = img.convertToFormat(QImage::Format_RGB888);
				} else {
					img = img.convertToFormat(QImage::Format_Grayscale8);
				}
				break;
			default:
				break; // RGB888, Grayscale8 etc. are fine
		}

		QString const dstName = QString::fromLatin1("%1%2.jpg")
			.arg(prefix)
			.arg(exported + 1, 3, 10, QLatin1Char('0'));
		QString const dstPath = dir.filePath(dstName);

		if (!img.save(dstPath, "JPEG", opts.quality)) {
			result.errorDetail = QObject::tr("Failed to save: %1").arg(dstPath);
			result.pageCount = -1;
			return result;
		}

		result.totalSize += QFileInfo(dstPath).size();
		++exported;
	}

	if (exported == 0) {
		result.errorDetail = QObject::tr(
			"No processed output images found. "
			"Run batch processing before exporting.");
		result.pageCount = -1;
		return result;
	}

	// Generate optional HTML viewer
	if (opts.generateViewer) {
		generateHtmlViewer(outputDir, exported, prefix);
	}

	result.pageCount = exported;
	return result;
}
