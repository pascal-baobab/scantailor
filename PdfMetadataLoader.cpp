/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "config.h"

#ifdef HAVE_POPPLER

#include "PdfMetadataLoader.h"
#include "ImageMetadata.h"
#include "Dpi.h"
#include <QIODevice>
#include <QFile>
#include <QSizeF>
#include <QSize>
#include <poppler-qt5.h>

void
PdfMetadataLoader::registerMyself()
{
	static bool registered = false;
	if (!registered) {
		ImageMetadataLoader::registerLoader(
			IntrusivePtr<ImageMetadataLoader>(new PdfMetadataLoader)
		);
		registered = true;
	}
}

ImageMetadataLoader::Status
PdfMetadataLoader::loadMetadata(
	QIODevice& io_device,
	VirtualFunction1<void, ImageMetadata const&>& out)
{
	// Check for PDF magic bytes: %PDF
	char magic[4];
	if (io_device.peek(magic, 4) != 4) {
		return FORMAT_NOT_RECOGNIZED;
	}
	if (memcmp(magic, "%PDF", 4) != 0) {
		return FORMAT_NOT_RECOGNIZED;
	}

	// We need the file path for Poppler.
	QFile* file = dynamic_cast<QFile*>(&io_device);
	if (!file) {
		return GENERIC_ERROR;
	}

	QString const filePath = file->fileName();
	Poppler::Document* doc = Poppler::Document::load(filePath);
	if (!doc || doc->isLocked()) {
		delete doc;
		return GENERIC_ERROR;
	}

	int const numPages = doc->numPages();
	if (numPages <= 0) {
		delete doc;
		return NO_IMAGES;
	}

	// Emit metadata for each page.
	// Poppler reports page size in points (1 point = 1/72 inch).
	int const renderDpi = 300;
	for (int i = 0; i < numPages; ++i) {
		Poppler::Page* page = doc->page(i);
		if (!page) {
			continue;
		}
		QSizeF const pageSizePt = page->pageSizeF();
		// Size in pixels at renderDpi
		int const widthPx = qRound(pageSizePt.width() * renderDpi / 72.0);
		int const heightPx = qRound(pageSizePt.height() * renderDpi / 72.0);
		delete page;

		QSize const size(widthPx, heightPx);
		Dpi const dpi(renderDpi, renderDpi);
		out(ImageMetadata(size, dpi));
	}

	delete doc;
	return LOADED;
}

#endif // HAVE_POPPLER
