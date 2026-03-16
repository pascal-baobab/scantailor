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

#ifndef RAG_EXPORTER_H_
#define RAG_EXPORTER_H_

#include <QString>

class ProjectPages;
class OutputFileNameGenerator;

/**
 * Exports processed ScanTailor pages as a set of image files plus a
 * machine-readable manifest.json for ingestion into a RAG (Retrieval-
 * Augmented Generation) system such as SCIENCE-RAG / Corso.
 *
 * Output structure:
 *   <outputDir>/
 *     manifest.json       — page metadata index (SCIENCE-RAG schema v1.0)
 *     page_000.png        — processed page images
 *     page_001.png
 *     ...
 */
class RagExporter
{
public:
    struct Options {
        int  dpi;              ///< Nominal DPI written to manifest (informational)
        bool useJpeg;          ///< false = PNG (lossless), true = JPEG

        Options() : dpi(300), useJpeg(false) {}
    };

    /**
     * Export the project.
     *
     * \param pages        The project pages (source of page list and metadata).
     * \param fileNameGen  Knows where the output-filter images live on disk.
     * \param outputDir    Destination directory (created if absent).
     * \param projectName  Written into the manifest (basename of .ScanTailor).
     * \param opts         Export options.
     * \return Empty string on success; human-readable error message on failure.
     */
    static QString exportProject(
        ProjectPages const& pages,
        OutputFileNameGenerator const& fileNameGen,
        QString const& outputDir,
        QString const& projectName,
        Options const& opts = Options());
};

#endif // RAG_EXPORTER_H_
