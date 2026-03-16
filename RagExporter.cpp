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

#include "RagExporter.h"
#include "ProjectPages.h"
#include "OutputFileNameGenerator.h"
#include "PageSequence.h"
#include "PageInfo.h"
#include "PageId.h"
#include "ImageId.h"
#include "ImageMetadata.h"
#include "PageView.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QString>
#include <QTextStream>

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static QString jsonEscape(QString const& s)
{
    QString out;
    out.reserve(s.size() + 4);
    for (int i = 0; i < s.size(); ++i) {
        QChar c = s.at(i);
        if (c == QLatin1Char('"'))       out += QLatin1String("\\\"");
        else if (c == QLatin1Char('\\')) out += QLatin1String("\\\\");
        else if (c == QLatin1Char('\n')) out += QLatin1String("\\n");
        else if (c == QLatin1Char('\r')) out += QLatin1String("\\r");
        else if (c == QLatin1Char('\t')) out += QLatin1String("\\t");
        else                             out += c;
    }
    return out;
}

// ---------------------------------------------------------------------------
// RagExporter::exportProject
// ---------------------------------------------------------------------------

QString RagExporter::exportProject(
    ProjectPages const& pages,
    OutputFileNameGenerator const& fileNameGen,
    QString const& outputDir,
    QString const& projectName,
    Options const& opts)
{
    // 1. Ensure output directory exists.
    QDir dir(outputDir);
    if (!dir.mkpath(QLatin1String("."))) {
        return QObject::tr("Cannot create output directory: %1").arg(outputDir);
    }

    PageSequence const seq = pages.toPageSequence(PAGE_VIEW);
    size_t const total = seq.numPages();

    // 2. Collect page records and copy images.
    // We build the JSON body as a string so there is no dependency on Qt5's
    // QJsonDocument — the project targets Qt4.
    QString pagesJson;
    QTextStream ps(&pagesJson);

    char const* fmt = opts.useJpeg ? "JPEG" : "PNG";
    char const* ext = opts.useJpeg ? "jpg"  : "png";

    size_t exported = 0;
    for (size_t i = 0; i < total; ++i) {
        PageInfo const& info    = seq.pageAt(i);
        PageId   const& pageId  = info.id();
        ImageId  const& imageId = pageId.imageId();

        // Source: the processed output image produced by the Output filter.
        QString const srcPath = fileNameGen.filePathFor(pageId);
        QImage img(srcPath);
        if (img.isNull()) {
            // Output not yet generated — skip but continue.
            continue;
        }

        // Destination filename: page_000.png / page_000.jpg
        QString const dstName = QString::fromLatin1("page_%1.%2")
            .arg(i, 3, 10, QLatin1Char('0'))
            .arg(QLatin1String(ext));
        QString const dstPath = dir.filePath(dstName);

        if (!img.save(dstPath, fmt)) {
            return QObject::tr("Failed to save image: %1").arg(dstPath);
        }

        // Build JSON entry for this page.
        ImageMetadata const& meta = info.metadata();
        QString const pageIdStr = QString::fromLatin1("%1::%2")
            .arg(jsonEscape(QFileInfo(imageId.filePath()).fileName()))
            .arg(i);

        if (exported > 0) {
            ps << QLatin1String(",\n");
        }
        ps << QLatin1String("    {\n");
        ps << QString::fromLatin1("      \"index\": %1,\n").arg(static_cast<int>(i));
        ps << QString::fromLatin1("      \"page_id\": \"%1\",\n").arg(pageIdStr);
        ps << QString::fromLatin1("      \"image_file\": \"%1\",\n")
              .arg(jsonEscape(dstName));
        ps << QString::fromLatin1("      \"original_source\": \"%1\",\n")
              .arg(jsonEscape(QFileInfo(imageId.filePath()).fileName()));
        ps << QString::fromLatin1("      \"width_px\": %1,\n").arg(img.width());
        ps << QString::fromLatin1("      \"height_px\": %1,\n").arg(img.height());
        ps << QString::fromLatin1("      \"source_dpi_x\": %1,\n")
              .arg(meta.dpi().horizontal());
        ps << QString::fromLatin1("      \"source_dpi_y\": %1\n")
              .arg(meta.dpi().vertical());
        ps << QLatin1String("    }");

        ++exported;
    }

    // 3. Write manifest.json.
    QString manifestPath = dir.filePath(QLatin1String("manifest.json"));
    QFile manifestFile(manifestPath);
    if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return QObject::tr("Cannot write manifest: %1").arg(manifestPath);
    }

    QTextStream ms(&manifestFile);
    ms.setCodec("UTF-8");
    ms << QLatin1String("{\n");
    ms << QLatin1String("  \"schema_version\": \"1.0\",\n");
    ms << QString::fromLatin1("  \"project_name\": \"%1\",\n")
          .arg(jsonEscape(projectName));
    ms << QLatin1String("  \"export_tool\": \"ScanTailor\",\n");
    ms << QString::fromLatin1("  \"page_count\": %1,\n")
          .arg(static_cast<int>(exported));
    ms << QLatin1String("  \"pages\": [\n");
    ms << pagesJson;
    ms << QLatin1String("\n  ]\n");
    ms << QLatin1String("}\n");
    manifestFile.close();

    if (exported == 0) {
        return QObject::tr(
            "No processed output images found. "
            "Run batch processing before exporting for RAG.");
    }

    return QString(); // success
}
