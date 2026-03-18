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

#include "TextExporter.h"
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

#ifdef HAVE_TESSERACT
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#endif

// ---------------------------------------------------------------------------
// Helper: find tessdata directory (shared logic with VectorPdfExporter)
// ---------------------------------------------------------------------------

static QString findTessDataDir(QString const& userPath, QString const& language)
{
	QStringList candidates;
	if (!userPath.isEmpty()) {
		if (userPath.endsWith("/tessdata") || userPath.endsWith("\\tessdata")) {
			candidates << userPath;
		} else if (userPath.endsWith("/tessdata/") || userPath.endsWith("\\tessdata\\")) {
			QString p = userPath;
			p.chop(1);
			candidates << p;
		} else {
			candidates << (userPath + "/tessdata");
		}
	}

	static QStringList const systemDirs = {
		"C:/msys64/mingw64/share/tessdata",
		"/usr/share/tessdata",
		"/usr/local/share/tessdata"
	};
	for (QString const& td : systemDirs) {
		if (QDir(td).exists()) {
			candidates << td;
		}
	}

	QString firstLang = language.split('+').first();
	for (QString const& c : candidates) {
		if (QFile::exists(c + "/" + firstLang + ".traineddata")) {
			return c;
		}
	}
	return QString();
}

// ---------------------------------------------------------------------------
// TextExporter::exportText
// ---------------------------------------------------------------------------

TextExporter::ExportResult TextExporter::exportText(
	IntrusivePtr<ProjectPages> const& pages,
	OutputFileNameGenerator const& fileNameGen,
	QString const& outputPath,
	Options const& opts,
	ProgressCallback progress)
{
	ExportResult result;
	result.pageCount = 0;
	result.wordCount = 0;
	result.charCount = 0;
	result.tessInitOk = false;

#ifndef HAVE_TESSERACT
	result.errorDetail = QObject::tr(
		"Tesseract OCR is not available (not compiled in).");
	result.pageCount = -1;
	return result;
#else

	PageSequence const seq = pages->toPageSequence(PAGE_VIEW);
	int const total = static_cast<int>(seq.numPages());

	if (total == 0) {
		result.errorDetail = QObject::tr("No pages in project.");
		result.pageCount = -1;
		return result;
	}

	// Find tessdata directory using smart resolution
	QString const tessDataDir = findTessDataDir(opts.tessDataPath, opts.language);
	QByteArray langBytes = opts.language.toUtf8();
	QByteArray pathBytes;
	char const* dataDir = nullptr;
	if (!tessDataDir.isEmpty()) {
		// Tesseract expects the PARENT of the tessdata/ folder
		QDir td(tessDataDir);
		td.cdUp();
		pathBytes = td.absolutePath().toUtf8();
		dataDir = pathBytes.constData();
	}

	// Initialize Tesseract
	tesseract::TessBaseAPI api;
	if (api.Init(dataDir, langBytes.constData()) != 0) {
		result.errorDetail = QObject::tr(
			"Failed to initialize Tesseract OCR.\n"
			"Language: %1\nData path: %2")
			.arg(opts.language)
			.arg(tessDataDir.isEmpty()
				? QObject::tr("(not found)") : tessDataDir);
		result.pageCount = -1;
		return result;
	}
	result.tessInitOk = true;
	api.SetVariable("tessedit_do_invert", "0");

	// For SINGLE_FILE mode, open the output file
	QFile singleFile;
	QTextStream singleStream;
	if (opts.mode == SINGLE_FILE) {
		singleFile.setFileName(outputPath);
		if (!singleFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			result.errorDetail = QObject::tr("Cannot create file: %1")
				.arg(outputPath);
			result.pageCount = -1;
			api.End();
			return result;
		}
		singleStream.setDevice(&singleFile);
		singleStream.setCodec("UTF-8");
	} else {
		// PER_PAGE_FILES: ensure directory exists
		QDir dir(outputPath);
		if (!dir.mkpath(QLatin1String("."))) {
			result.errorDetail = QObject::tr(
				"Cannot create output directory: %1").arg(outputPath);
			result.pageCount = -1;
			api.End();
			return result;
		}
	}

	int exported = 0;

	for (int i = 0; i < total; ++i) {
		if (progress && progress(i, total)) {
			api.End();
			if (opts.mode == SINGLE_FILE) {
				singleFile.close();
				QFile::remove(outputPath); // Clean up partial file
			}
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

		// Convert to grayscale for OCR
		QImage gray = img.convertToFormat(QImage::Format_Grayscale8);

		// Downscale to ~300 DPI for OCR if higher
		int ocrDpi = opts.dpi;
		double actualDpi = (img.dotsPerMeterX() > 0)
			? img.dotsPerMeterX() / 39.3701 : static_cast<double>(opts.dpi);
		if (actualDpi > 350) {
			double scale = 300.0 / actualDpi;
			int newW = static_cast<int>(gray.width() * scale);
			int newH = static_cast<int>(gray.height() * scale);
			if (newW > 0 && newH > 0) {
				gray = gray.scaled(newW, newH,
					Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				ocrDpi = 300;
			}
		}

		api.SetImage(gray.constBits(), gray.width(), gray.height(),
			1, gray.bytesPerLine());
		api.SetSourceResolution(ocrDpi > 0 ? ocrDpi : 300);

		char* text = api.GetUTF8Text();
		api.Clear(); // Free internal recognition data between pages

		if (!text) {
			continue;
		}

		QString pageText = QString::fromUtf8(text).trimmed();
		delete[] text;

		if (pageText.isEmpty()) {
			continue;
		}

		// Count words and characters
		int words = 0;
		bool inWord = false;
		for (int c = 0; c < pageText.size(); ++c) {
			if (pageText[c].isLetterOrNumber()) {
				if (!inWord) {
					++words;
					inWord = true;
				}
			} else {
				inWord = false;
			}
		}
		result.wordCount += words;
		result.charCount += pageText.size();

		if (opts.mode == SINGLE_FILE) {
			if (exported > 0) {
				if (opts.insertPageBreaks) {
					singleStream << QChar('\f');
				}
				singleStream << QLatin1String("\n");
			}
			if (opts.insertPageHeaders) {
				singleStream << QString::fromLatin1("--- Page %1 ---\n\n")
					.arg(i + 1);
			}
			singleStream << pageText << QLatin1String("\n");
		} else {
			// PER_PAGE_FILES mode
			QString const fname = QString::fromLatin1("page_%1.txt")
				.arg(i + 1, 3, 10, QLatin1Char('0'));
			QString const fpath = QDir(outputPath).filePath(fname);

			QFile pageFile(fpath);
			if (pageFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
				QTextStream ps(&pageFile);
				ps.setCodec("UTF-8");
				ps << pageText << QLatin1String("\n");
				pageFile.close();
			}
		}

		++exported;
	}

	api.End();

	if (opts.mode == SINGLE_FILE) {
		singleFile.close();
		if (exported == 0) {
			QFile::remove(outputPath); // Remove empty file
		}
	}

	if (exported == 0) {
		result.errorDetail = QObject::tr(
			"No text could be extracted. Ensure pages are processed "
			"and Tesseract language data is installed.");
		result.pageCount = -1;
		return result;
	}

	result.pageCount = exported;
	return result;

#endif // HAVE_TESSERACT
}
