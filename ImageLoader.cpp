/*
      Scan Tailor - Interactive post-processing tool for scanned pages.
      Copyright (C)  Joseph Artsimovich <joseph.artsimovigh@gmail.com>

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

#include "ImageLoader.h"
#include "ImageId.h"
#include "TiffReader.h"
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QImage>
#include <QString>

#ifdef HAVE_POPPLER
#include <poppler-qt5.h>
#endif

QImage ImageLoader::load(ImageId const &image_id) {
  return load(image_id.filePath(), image_id.zeroBasedPage());
}

QImage ImageLoader::load(QString const &file_path, int const page_num) {
#ifdef HAVE_POPPLER
  // Check if the file is a PDF by extension
  QFileInfo fileInfo(file_path);
  if (fileInfo.suffix().toLower() == QLatin1String("pdf")) {
    // Try to load the PDF with Poppler
    Poppler::Document *doc = Poppler::Document::load(file_path);
    if (doc && !doc->isLocked()) {
      if (page_num >= 0 && page_num < doc->numPages()) {
        Poppler::Page *page = doc->page(page_num);
        if (page) {
          // Render the page to an image at 300 DPI
          QImage image = page->renderToImage(300, 300);
          delete page;
          delete doc;
          if (!image.isNull()) {
            return image;
          }
        }
      }
      delete doc;
    }
    // If Poppler fails, fall back to regular image loading
  }
#endif

  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly)) {
    return QImage();
  }
  return load(file, page_num);
}

QImage ImageLoader::load(QIODevice &io_dev, int const page_num) {
  if (TiffReader::canRead(io_dev)) {
    return TiffReader::readImage(io_dev, page_num);
  }

  if (page_num != 0) {
    // Qt can only load the first page of multi-page images.
    return QImage();
  }

  QImage image;
  image.load(&io_dev, 0);
  return image;
}
