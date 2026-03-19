> **ABANDONED** — This plan was rejected. Potrace vectorization produced poor results.
> The chosen approach is JPEG+OCR via Tesseract (see memory `project_vectorized_pdf`).
> Kept for historical reference only.

# Potrace Vectorized PDF Export — Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** When "Generate PDF" is checked in the Output panel, batch processing produces TIFFs as usual, then compiles them into a vectorized PDF where text is traced to bezier curves via Potrace + OCR, with lossless image embedding — all triggered by the arrow button.

**Architecture:** The pipeline is: batch → TIFFs in `/out` → load TIFFs → for each page: threshold to binary → Potrace traces text to vector paths → Tesseract provides searchable text → write hybrid PDF (lossless raster background + vector text paths + invisible OCR layer). The PDF writer (`writeCompactPdf`) is refactored to support FlateDecode lossless embedding and Potrace vector path content streams. The Export panel is simplified to a single "Generate PDF" checkbox with OCR language, DPI, and size estimate options.

**Tech Stack:** C++/Qt5, Potrace (libpotrace via MSYS2 `mingw-w64-x86_64-potrace`), Tesseract OCR, custom PDF writer, BinaryImage for bitmap operations.

---

## File Structure

### New files:
- `PotraceVectorizer.h` — Class wrapping Potrace: QImage → vector bezier paths
- `PotraceVectorizer.cpp` — Implementation: bitmap conversion, tracing, PDF path generation

### Modified files:
- `CMakeLists.txt` — Add `ENABLE_POTRACE` option, find libpotrace
- `VectorPdfExporter.h` — New `exportVectorizedPdf()` method, `VectorPath` struct, lossless mode
- `VectorPdfExporter.cpp` — Hybrid PDF writer: FlateDecode images + vector paths + OCR text
- `filters/output/ui/OutputOptionsWidget.ui` — Simplified Export panel (one checkbox + options)
- `filters/output/OptionsWidget.h` — Cleaned signals (remove export buttons), add language/DPI signals
- `filters/output/OptionsWidget.cpp` — Wire new UI, remove old button connections, size estimate
- `MainWindow.h` — Clean up slots/members, add OCR language + DPI members
- `MainWindow.cpp` — New batch completion pipeline, remove old export methods
- `.github/workflows/build-windows.yml` — Add `mingw-w64-x86_64-potrace`, `ENABLE_POTRACE=ON`
- `.github/workflows/build-linux.yml` — Add potrace dev package
- `.github/workflows/build-macos.yml` — Add potrace via Homebrew

### Files to delete:
- `VectorPdfDialog.h` — No longer needed (options are inline in Export panel)
- `VectorPdfDialog.cpp` — No longer needed

---

## Chunk 1: CMake + Potrace Integration

### Task 1: Add Potrace to CMake build system

**Files:**
- Modify: `CMakeLists.txt:431-429` (after ENABLE_TESSERACT block)

- [ ] **Step 1: Add ENABLE_POTRACE option to CMakeLists.txt**

After the `ENDIF(ENABLE_TESSERACT)` block (~line 456), add:

```cmake
# Potrace vectorization for PDF export (bitmap → bezier curves)
OPTION(ENABLE_POTRACE "Enable text vectorization via Potrace for PDF export" OFF)
IF(ENABLE_POTRACE)
	FIND_PACKAGE(PkgConfig QUIET)
	IF(PkgConfig_FOUND)
		PKG_CHECK_MODULES(POTRACE potrace)
	ENDIF()
	IF(NOT POTRACE_FOUND)
		FIND_LIBRARY(POTRACE_LIBRARIES NAMES potrace REQUIRED)
		FIND_PATH(POTRACE_INCLUDE_DIRS potracelib.h)
		IF(NOT POTRACE_LIBRARIES OR NOT POTRACE_INCLUDE_DIRS)
			MESSAGE(FATAL_ERROR "Potrace not found. Install: pacman -S mingw-w64-x86_64-potrace")
		ENDIF()
		SET(POTRACE_FOUND TRUE)
	ENDIF()
	ADD_DEFINITIONS(-DHAVE_POTRACE)
	INCLUDE_DIRECTORIES(${POTRACE_INCLUDE_DIRS})
	SET(EXTRA_LIBS ${EXTRA_LIBS} ${POTRACE_LIBRARIES})
ENDIF(ENABLE_POTRACE)
```

- [ ] **Step 2: Add PotraceVectorizer source files to stcore target**

Find the `SET(common_sources ...)` block and add:

```cmake
PotraceVectorizer.cpp
PotraceVectorizer.h
```

- [ ] **Step 3: Verify CMake configure works with Potrace**

Run:
```bash
PATH="/c/msys64/mingw64/bin:$PATH"
pacman -S --needed mingw-w64-x86_64-potrace
cmake -S . -B build-qt5 -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/mingw64 \
  -DENABLE_POPPLER=ON \
  -DENABLE_TESSERACT=ON \
  -DENABLE_POTRACE=ON
```
Expected: configure succeeds, `HAVE_POTRACE` defined.

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt
git commit -m "feat: add ENABLE_POTRACE CMake option for text vectorization"
```

---

### Task 2: Create PotraceVectorizer class

**Files:**
- Create: `PotraceVectorizer.h`
- Create: `PotraceVectorizer.cpp`

- [ ] **Step 1: Write PotraceVectorizer.h**

```cpp
/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#ifndef POTRACE_VECTORIZER_H_
#define POTRACE_VECTORIZER_H_

#include <QImage>
#include <QByteArray>
#include <QList>

/**
 * Wraps libpotrace to convert a raster image to vector bezier paths.
 * Output is a PDF content stream fragment (moveto/lineto/curveto commands).
 */
class PotraceVectorizer
{
public:
    struct Options {
        int turdSize;       ///< Remove speckles up to this size (default 2)
        double alphaMax;    ///< Corner threshold 0.0-1.34 (default 1.0)
        int threshold;      ///< B&W threshold 0-255 (default 128)

        Options() : turdSize(2), alphaMax(1.0), threshold(128) {}
    };

    /**
     * Trace a raster image to vector paths and return a PDF content stream
     * fragment containing the path drawing operators (m, l, c, h, f).
     *
     * @param image     Input image (any format, will be thresholded to B&W)
     * @param pageW     PDF page width in points (for coordinate scaling)
     * @param pageH     PDF page height in points (for coordinate scaling)
     * @param dpi       Image resolution (pixels per inch)
     * @param opts      Tracing options
     * @return PDF content stream bytes (path operators), empty if tracing fails
     */
    static QByteArray traceToPathStream(
        QImage const& image,
        double pageW, double pageH,
        int dpi,
        Options const& opts = Options()
    );

private:
    PotraceVectorizer() = delete;
};

#endif // POTRACE_VECTORIZER_H_
```

- [ ] **Step 2: Write PotraceVectorizer.cpp**

```cpp
/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "PotraceVectorizer.h"

#ifdef HAVE_POTRACE
#include <potracelib.h>
#endif

#include <QColor>
#include <cstdlib>
#include <cstring>

#ifdef HAVE_POTRACE

QByteArray
PotraceVectorizer::traceToPathStream(
    QImage const& image,
    double pageW, double pageH,
    int dpi,
    Options const& opts)
{
    QByteArray result;

    if (image.isNull()) {
        return result;
    }

    int const w = image.width();
    int const h = image.height();

    // --- Convert QImage to Potrace bitmap ---
    // Potrace expects: 1 = foreground (black), 0 = background (white)
    // Bits packed into unsigned long words, MSB = leftmost pixel.
    // Scanlines go top-to-bottom (we handle the Y-flip when emitting PDF coords).

    int const wordsPerLine = (w + (int)(sizeof(potrace_word) * 8) - 1)
                           / (int)(sizeof(potrace_word) * 8);
    int const bitmapSize = wordsPerLine * h;

    potrace_word* bitmapData = (potrace_word*)std::calloc(bitmapSize, sizeof(potrace_word));
    if (!bitmapData) {
        return result;
    }

    // Threshold image to binary and pack into potrace_word array.
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    for (int y = 0; y < h; ++y) {
        uchar const* line = gray.constScanLine(y);
        potrace_word* dst = bitmapData + y * wordsPerLine;
        for (int x = 0; x < w; ++x) {
            // pixel < threshold → foreground (1 = black text)
            if (line[x] < opts.threshold) {
                int const wordIdx = x / (int)(sizeof(potrace_word) * 8);
                int const bitIdx = (int)(sizeof(potrace_word) * 8) - 1
                                 - (x % (int)(sizeof(potrace_word) * 8));
                dst[wordIdx] |= ((potrace_word)1 << bitIdx);
            }
        }
    }

    // --- Set up Potrace bitmap struct ---
    potrace_bitmap_t bm;
    bm.w = w;
    bm.h = h;
    bm.dy = wordsPerLine;
    bm.map = bitmapData;

    // --- Configure Potrace parameters ---
    potrace_param_t* param = potrace_param_default();
    if (!param) {
        std::free(bitmapData);
        return result;
    }
    param->turdsize = opts.turdSize;
    param->alphamax = opts.alphaMax;

    // --- Trace ---
    potrace_state_t* state = potrace_trace(param, &bm);
    potrace_param_free(param);
    std::free(bitmapData);

    if (!state || state->status != POTRACE_STATUS_OK) {
        if (state) potrace_state_free(state);
        return result;
    }

    // --- Convert traced paths to PDF content stream ---
    // Coordinate transform: Potrace pixel (0,0) = top-left
    // PDF (0,0) = bottom-left, units = points
    double const scaleX = pageW / w;
    double const scaleY = pageH / h;

    // Set fill color to black for text paths.
    result += "q\n";
    result += "0 0 0 rg\n";  // RGB black fill

    potrace_path_t* path = state->plist;
    while (path) {
        potrace_curve_t const& curve = path->curve;
        int const n = curve.n;

        if (n < 1) {
            path = path->next;
            continue;
        }

        // Start point: last segment's endpoint (curves are closed).
        potrace_dpoint_t const& start = curve.c[n - 1][2];
        double sx = start.x * scaleX;
        double sy = pageH - start.y * scaleY;  // flip Y

        result += QByteArray::number(sx, 'f', 2) + " "
                + QByteArray::number(sy, 'f', 2) + " m\n";

        for (int i = 0; i < n; ++i) {
            if (curve.tag[i] == POTRACE_CURVETO) {
                double const x1 = curve.c[i][0].x * scaleX;
                double const y1 = pageH - curve.c[i][0].y * scaleY;
                double const x2 = curve.c[i][1].x * scaleX;
                double const y2 = pageH - curve.c[i][1].y * scaleY;
                double const x3 = curve.c[i][2].x * scaleX;
                double const y3 = pageH - curve.c[i][2].y * scaleY;

                result += QByteArray::number(x1, 'f', 2) + " "
                        + QByteArray::number(y1, 'f', 2) + " "
                        + QByteArray::number(x2, 'f', 2) + " "
                        + QByteArray::number(y2, 'f', 2) + " "
                        + QByteArray::number(x3, 'f', 2) + " "
                        + QByteArray::number(y3, 'f', 2) + " c\n";
            } else {
                // POTRACE_CORNER: two straight line segments
                // corner point = c[i][1], endpoint = c[i][2]
                double const cx = curve.c[i][1].x * scaleX;
                double const cy = pageH - curve.c[i][1].y * scaleY;
                double const ex = curve.c[i][2].x * scaleX;
                double const ey = pageH - curve.c[i][2].y * scaleY;

                result += QByteArray::number(cx, 'f', 2) + " "
                        + QByteArray::number(cy, 'f', 2) + " l\n";
                result += QByteArray::number(ex, 'f', 2) + " "
                        + QByteArray::number(ey, 'f', 2) + " l\n";
            }
        }

        // Close and fill path.
        // Potrace uses even-odd fill rule for nested paths (holes).
        result += "h\n";

        // Use 'f*' (even-odd fill) to handle holes correctly.
        // But we accumulate all paths and fill once at the end.
        // Actually, each path needs individual fill for correct hole handling.
        if (path->sign == '+') {
            result += "f*\n";  // Outer path: fill
        } else {
            // Inner path (hole): will be handled by even-odd rule
            // when combined with parent. For simplicity, fill each.
            result += "f*\n";
        }

        path = path->next;
    }

    result += "Q\n";

    potrace_state_free(state);
    return result;
}

#else // !HAVE_POTRACE

QByteArray
PotraceVectorizer::traceToPathStream(
    QImage const&, double, double, int, Options const&)
{
    return QByteArray();
}

#endif // HAVE_POTRACE
```

- [ ] **Step 3: Compile and verify**

```bash
PATH="/c/msys64/mingw64/bin:$PATH"
cmake --build build-qt5 --parallel 2>&1 | tail -20
```
Expected: compiles without errors.

- [ ] **Step 4: Commit**

```bash
git add PotraceVectorizer.h PotraceVectorizer.cpp
git commit -m "feat: add PotraceVectorizer class for bitmap-to-bezier tracing"
```

---

## Chunk 2: Refactor PDF Writer for Lossless + Vector Paths

### Task 3: Refactor VectorPdfExporter to support lossless embedding and vector paths

**Files:**
- Modify: `VectorPdfExporter.h`
- Modify: `VectorPdfExporter.cpp`

- [ ] **Step 1: Add new structs and method to VectorPdfExporter.h**

Add a `VectorPath` data holder and a new `exportVectorizedPdf()` method. Replace the entire file:

In `VectorPdfExporter.h`, add after the `OcrWord` struct (make it public along with a new struct):

Move `OcrWord` to public section and add:

```cpp
public:
    struct OcrWord {
        QString text;
        int x, y, w, h;
        float confidence;
    };

    struct PageData {
        QImage image;               ///< Full raster image of the page
        QByteArray vectorPaths;     ///< PDF path stream from Potrace (may be empty)
        QList<OcrWord> ocrWords;    ///< OCR words for searchable text (may be empty)
    };

    /**
     * Export a vectorized PDF: lossless raster + vector text paths + OCR overlay.
     *
     * @param pages          Project pages
     * @param fileNameGen    Maps page IDs to output file paths
     * @param outputPdfPath  Where to write the PDF
     * @param opts           Options (language, dpi, jpegQuality — jpegQuality ignored if lossless)
     * @param lossless       If true, embed images with FlateDecode; if false, use JPEG
     * @param vectorize      If true, run Potrace on each page for vector text paths
     * @param progress       Optional progress callback
     * @return number of pages exported, or -1 on error
     */
    static int exportVectorizedPdf(
        IntrusivePtr<ProjectPages> const& pages,
        OutputFileNameGenerator const& fileNameGen,
        QString const& outputPdfPath,
        Options const& opts,
        bool lossless,
        bool vectorize,
        ProgressCallback progress = ProgressCallback()
    );

private:
    /// Write PDF with optional lossless images, vector paths, and OCR text.
    static bool writeHybridPdf(
        QString const& outputPdfPath,
        QList<PageData> const& pages,
        Options const& opts,
        bool lossless
    );
```

Remove the old `exportCompactPdf()` declaration (it will be replaced by `exportVectorizedPdf`).

- [ ] **Step 2: Implement writeHybridPdf() in VectorPdfExporter.cpp**

This is the new core PDF writer. It replaces `writeCompactPdf` with support for:
- FlateDecode (lossless) or DCTDecode (JPEG) image embedding
- Vector path content stream from Potrace
- Invisible OCR text overlay

The key change from `writeCompactPdf` is in the image object and content stream:

For **lossless FlateDecode**, replace the JPEG block (lines 75-98) with:

```cpp
if (lossless) {
    // Lossless: raw RGB pixels compressed with zlib (FlateDecode).
    QImage rgbImg = pageData.image.convertToFormat(QImage::Format_RGB888);
    int const imgW = rgbImg.width();
    int const imgH = rgbImg.height();

    // Collect raw RGB bytes (no padding — PDF needs continuous data).
    QByteArray rawData;
    rawData.reserve(imgW * imgH * 3);
    for (int y = 0; y < imgH; ++y) {
        uchar const* line = rgbImg.constScanLine(y);
        rawData.append(reinterpret_cast<char const*>(line), imgW * 3);
    }

    // Compress with zlib.
    QByteArray compressed = qCompress(rawData, 9);
    // qCompress prepends a 4-byte big-endian size header — strip it for PDF.
    QByteArray zlibData = compressed.mid(4);

    QByteArray imgObj;
    imgObj += "<< /Type /XObject /Subtype /Image";
    imgObj += " /Width " + QByteArray::number(imgW);
    imgObj += " /Height " + QByteArray::number(imgH);
    imgObj += " /ColorSpace /DeviceRGB";
    imgObj += " /BitsPerComponent 8";
    imgObj += " /Filter /FlateDecode";
    imgObj += " /Length " + QByteArray::number(zlibData.size());
    imgObj += " >>\nstream\n";
    imgObj += zlibData;
    imgObj += "\nendstream";
    // ...
} else {
    // JPEG: existing DCTDecode code
    // ...
}
```

For the **content stream**, add vector paths between image and OCR text:

```cpp
// 1. Draw raster image (background layer).
content += "q\n";
content += QByteArray::number(pageW, 'f', 2) + " 0 0 "
         + QByteArray::number(pageH, 'f', 2) + " 0 0 cm\n";
content += "/Img Do\n";
content += "Q\n";

// 2. Vector text paths from Potrace (if available).
if (!pageData.vectorPaths.isEmpty()) {
    content += pageData.vectorPaths;
}

// 3. Invisible OCR text overlay (searchable/selectable).
// ... existing invisible text code ...
```

- [ ] **Step 3: Implement exportVectorizedPdf()**

```cpp
int
VectorPdfExporter::exportVectorizedPdf(
    IntrusivePtr<ProjectPages> const& pages,
    OutputFileNameGenerator const& fileNameGen,
    QString const& outputPdfPath,
    Options const& opts,
    bool lossless,
    bool vectorize,
    ProgressCallback progress)
{
    PageSequence const seq = pages->toPageSequence(PAGE_VIEW);
    int const totalPages = static_cast<int>(seq.numPages());
    if (totalPages == 0) return 0;

    QDir().mkpath(QFileInfo(outputPdfPath).absolutePath());

    QList<PageData> allPages;

    for (int i = 0; i < totalPages; ++i) {
        if (progress && progress(i, totalPages)) break;

        PageInfo const& info = seq.pageAt(i);
        QString const imgPath = fileNameGen.filePathFor(info.id());
        QImage img(imgPath);
        if (img.isNull()) continue;

        PageData pd;
        pd.image = img;

        // Vectorize text regions with Potrace.
        if (vectorize) {
            double const pageW = (img.width() / (double)opts.dpi) * 72.0;
            double const pageH = (img.height() / (double)opts.dpi) * 72.0;
            pd.vectorPaths = PotraceVectorizer::traceToPathStream(
                img, pageW, pageH, opts.dpi);
        }

        // OCR for searchable text layer.
        pd.ocrWords = ocrImage(img, opts);

        allPages.append(pd);
    }

    if (allPages.isEmpty()) return 0;

    if (!writeHybridPdf(outputPdfPath, allPages, opts, lossless)) {
        return -1;
    }

    if (progress) progress(totalPages, totalPages);
    return allPages.size();
}
```

- [ ] **Step 4: Remove old methods that are no longer needed**

Remove from `.h` and `.cpp`:
- `exportCompactPdf()` — replaced by `exportVectorizedPdf()`
- `exportSearchablePdf()` — replaced by `exportVectorizedPdf()`
- `writeCompactPdf()` — replaced by `writeHybridPdf()`

Keep:
- `vectorizeImages()` and `vectorizePdf()` — standalone tools, still useful
- `ocrImage()` — used by the new method

- [ ] **Step 5: Build and verify**

```bash
PATH="/c/msys64/mingw64/bin:$PATH"
cmake --build build-qt5 --parallel 2>&1 | tail -20
```

- [ ] **Step 6: Commit**

```bash
git add VectorPdfExporter.h VectorPdfExporter.cpp
git commit -m "feat: refactor PDF writer for lossless embedding + Potrace vector paths"
```

---

## Chunk 3: Simplify Export Panel UI

### Task 4: Redesign the Export panel in OutputOptionsWidget.ui

**Files:**
- Modify: `filters/output/ui/OutputOptionsWidget.ui`

The Export panel should contain only:

```
Export
  [x] Generate PDF

  OCR Language:
  [x] English  [x] Italian  [ ] French  [ ] German  [ ] Spanish

  DPI: [300 ▾]

  Est. size: ~XXX KB/page (A5)
```

- [ ] **Step 1: Replace the entire Export group in OutputOptionsWidget.ui**

Remove ALL existing export widgets (autoGeneratePdfCB, jpegQualitySlider, jpegQualitySpin, pdfSizeEstLabel, autoVectorizePdfCB, exportImagesBtn, exportPdfBtn, exportBothBtn, vectorizePdfBtn).

Replace the `<widget class="QGroupBox" name="exportPanel">` content with:

```xml
<widget class="QGroupBox" name="exportPanel">
 <property name="title">
  <string>Export</string>
 </property>
 <layout class="QVBoxLayout" name="verticalLayout_export">
  <item>
   <widget class="QCheckBox" name="generatePdfCB">
    <property name="text">
     <string>Generate PDF</string>
    </property>
    <property name="toolTip">
     <string>After batch processing, compile TIFFs into a vectorized PDF with searchable text</string>
    </property>
   </widget>
  </item>
  <item>
   <widget class="QWidget" name="pdfOptionsPanel">
    <layout class="QVBoxLayout" name="pdfOptionsLayout">
     <property name="leftMargin"><number>12</number></property>
     <property name="topMargin"><number>0</number></property>
     <property name="rightMargin"><number>0</number></property>
     <property name="bottomMargin"><number>0</number></property>
     <item>
      <widget class="QLabel" name="ocrLangLabel">
       <property name="text">
        <string>OCR Language:</string>
       </property>
      </widget>
     </item>
     <item>
      <layout class="QGridLayout" name="langGrid">
       <item row="0" column="0">
        <widget class="QCheckBox" name="langEngCB">
         <property name="text"><string>English</string></property>
         <property name="checked"><bool>true</bool></property>
        </widget>
       </item>
       <item row="0" column="1">
        <widget class="QCheckBox" name="langItaCB">
         <property name="text"><string>Italian</string></property>
        </widget>
       </item>
       <item row="1" column="0">
        <widget class="QCheckBox" name="langFraCB">
         <property name="text"><string>French</string></property>
        </widget>
       </item>
       <item row="1" column="1">
        <widget class="QCheckBox" name="langDeuCB">
         <property name="text"><string>German</string></property>
        </widget>
       </item>
       <item row="2" column="0">
        <widget class="QCheckBox" name="langSpaCB">
         <property name="text"><string>Spanish</string></property>
        </widget>
       </item>
       <item row="2" column="1">
        <widget class="QCheckBox" name="langRusCB">
         <property name="text"><string>Russian</string></property>
        </widget>
       </item>
      </layout>
     </item>
     <item>
      <layout class="QHBoxLayout" name="dpiLayout">
       <item>
        <widget class="QLabel" name="pdfDpiLabel">
         <property name="text"><string>DPI:</string></property>
        </widget>
       </item>
       <item>
        <widget class="QSpinBox" name="pdfDpiSpin">
         <property name="minimum"><number>72</number></property>
         <property name="maximum"><number>600</number></property>
         <property name="value"><number>300</number></property>
         <property name="suffix"><string> dpi</string></property>
        </widget>
       </item>
       <item>
        <spacer><property name="orientation"><enum>Qt::Horizontal</enum></property></spacer>
       </item>
      </layout>
     </item>
     <item>
      <widget class="QLabel" name="pdfSizeEstLabel">
       <property name="text"><string/></property>
       <property name="textFormat"><enum>Qt::RichText</enum></property>
      </widget>
     </item>
    </layout>
   </widget>
  </item>
 </layout>
</widget>
```

- [ ] **Step 2: Commit**

```bash
git add filters/output/ui/OutputOptionsWidget.ui
git commit -m "feat: simplify Export panel to single Generate PDF checkbox with options"
```

---

### Task 5: Update OptionsWidget signals and connections

**Files:**
- Modify: `filters/output/OptionsWidget.h`
- Modify: `filters/output/OptionsWidget.cpp`

- [ ] **Step 1: Clean up OptionsWidget.h signals**

Remove these signals:
- `exportImagesRequested()`
- `exportPdfRequested()`
- `exportBothRequested()`
- `autoGeneratePdfChanged(bool)`
- `jpegQualityChanged(int)`
- `autoVectorizePdfChanged(bool)`
- `vectorizePdfRequested()`

Add these signals:
```cpp
signals:
    // ... keep existing despeckleLevel/depthPerception signals ...

    void generatePdfChanged(bool enabled);
    void ocrLanguageChanged(QString const& langCode);
    void pdfDpiChanged(int dpi);
```

Remove old slots: `jpegQualitySliderChanged`, `updatePdfSizeEstimate`.

Add new slots:
```cpp
private slots:
    // ... keep all existing slots ...
    void generatePdfToggled(bool checked);
    void ocrLangToggled();
    void pdfDpiValueChanged(int val);
    void updatePdfSizeEstimate();
```

- [ ] **Step 2: Update OptionsWidget.cpp constructor connections**

Remove all old export button/checkbox connections. Add:

```cpp
connect(generatePdfCB, SIGNAL(toggled(bool)),
    this, SLOT(generatePdfToggled(bool)));
connect(langEngCB, SIGNAL(toggled(bool)), this, SLOT(ocrLangToggled()));
connect(langItaCB, SIGNAL(toggled(bool)), this, SLOT(ocrLangToggled()));
connect(langFraCB, SIGNAL(toggled(bool)), this, SLOT(ocrLangToggled()));
connect(langDeuCB, SIGNAL(toggled(bool)), this, SLOT(ocrLangToggled()));
connect(langSpaCB, SIGNAL(toggled(bool)), this, SLOT(ocrLangToggled()));
connect(langRusCB, SIGNAL(toggled(bool)), this, SLOT(ocrLangToggled()));
connect(pdfDpiSpin, SIGNAL(valueChanged(int)),
    this, SLOT(pdfDpiValueChanged(int)));

// Initially hide PDF options if checkbox is unchecked.
pdfOptionsPanel->setVisible(generatePdfCB->isChecked());

updatePdfSizeEstimate();
```

- [ ] **Step 3: Implement the new slots**

```cpp
void OptionsWidget::generatePdfToggled(bool const checked)
{
    pdfOptionsPanel->setVisible(checked);
    emit generatePdfChanged(checked);
}

void OptionsWidget::ocrLangToggled()
{
    QStringList codes;
    if (langEngCB->isChecked()) codes << "eng";
    if (langItaCB->isChecked()) codes << "ita";
    if (langFraCB->isChecked()) codes << "fra";
    if (langDeuCB->isChecked()) codes << "deu";
    if (langSpaCB->isChecked()) codes << "spa";
    if (langRusCB->isChecked()) codes << "rus";
    if (codes.isEmpty()) codes << "eng"; // fallback
    emit ocrLanguageChanged(codes.join("+"));
}

void OptionsWidget::pdfDpiValueChanged(int const val)
{
    updatePdfSizeEstimate();
    emit pdfDpiChanged(val);
}

void OptionsWidget::updatePdfSizeEstimate()
{
    double const dpiVal = pdfDpiSpin->value();

    // A5 page: 5.83 x 8.27 inches.
    double const pxW = 5.83 * dpiVal;
    double const pxH = 8.27 * dpiVal;
    // Lossless FlateDecode: ~40-60% of raw size for typical scans.
    // After vectorization: text replaced by vectors, raster shrinks ~30-50%.
    double const rawMB = pxW * pxH * 3.0 / (1024.0 * 1024.0);
    double const compressedMB = rawMB * 0.35; // ~35% of raw after FlateDecode + vectorization

    QString est;
    if (compressedMB < 1.0)
        est = tr("~%1 KB / page (A5)").arg(int(compressedMB * 1024));
    else
        est = tr("~%1 MB / page (A5)").arg(compressedMB, 0, 'f', 1);

    QString color = compressedMB < 1.0 ? "#4CAF50"
                  : (compressedMB < 3.0 ? "#FF8C00" : "#CC3333");
    pdfSizeEstLabel->setText(
        QString("<small><span style='color:%1'>%2</span></small>").arg(color, est));
}
```

- [ ] **Step 4: Remove old slot implementations**

Delete the old `jpegQualitySliderChanged()` and old `updatePdfSizeEstimate()` implementations.

- [ ] **Step 5: Build and verify**

- [ ] **Step 6: Commit**

```bash
git add filters/output/OptionsWidget.h filters/output/OptionsWidget.cpp
git commit -m "feat: wire new Export panel signals (generate PDF, OCR lang, DPI)"
```

---

## Chunk 4: MainWindow Pipeline + Cleanup

### Task 6: Refactor MainWindow for the new pipeline

**Files:**
- Modify: `MainWindow.h`
- Modify: `MainWindow.cpp`

- [ ] **Step 1: Clean up MainWindow.h**

Remove slots:
- `exportImagesTriggered()`
- `exportBothTriggered()`
- `exportPdfTriggered()` (from public slots too)
- `autoGeneratePdfToggled(bool)`
- `jpegQualityChanged(int)`
- `autoVectorizePdfToggled(bool)`
- `vectorizePdfTriggered()`
- `vectorizePdfStandalone()`

Remove private methods:
- `doExportImages()`
- `doExportPdf()`

Remove members:
- `m_autoGeneratePdf`
- `m_pdfJpegQuality`
- `m_autoVectorizePdf`

Add new slots:
```cpp
private slots:
    void generatePdfToggled(bool enabled);
    void ocrLanguageChanged(QString const& langCode);
    void pdfDpiChanged(int dpi);
```

Add new members:
```cpp
    bool m_generatePdf;
    QString m_ocrLanguage;
    int m_pdfDpi;
```

- [ ] **Step 2: Update MainWindow.cpp constructor**

Initialize new members:
```cpp
m_generatePdf(false),
m_ocrLanguage("eng"),
m_pdfDpi(300)
```

- [ ] **Step 3: Update signal connections in setOptionsWidget()**

Remove all old export signal connections. Add:
```cpp
connect(widget, SIGNAL(generatePdfChanged(bool)),
    this, SLOT(generatePdfToggled(bool)));
connect(widget, SIGNAL(ocrLanguageChanged(QString const&)),
    this, SLOT(ocrLanguageChanged(QString const&)));
connect(widget, SIGNAL(pdfDpiChanged(int)),
    this, SLOT(pdfDpiChanged(int)));
```

- [ ] **Step 4: Implement new slots**

```cpp
void MainWindow::generatePdfToggled(bool const enabled)
{
    m_generatePdf = enabled;
}

void MainWindow::ocrLanguageChanged(QString const& langCode)
{
    m_ocrLanguage = langCode;
}

void MainWindow::pdfDpiChanged(int const dpi)
{
    m_pdfDpi = dpi;
}
```

- [ ] **Step 5: Rewrite batch completion to use new pipeline**

Replace the auto-generate PDF block in `filterResult()`:

```cpp
// Generate vectorized PDF after output batch completes.
if (wasOutputFilter && m_generatePdf) {
    QString const outDir = m_outFileNameGen.outDir();
    QString const pdfDir = QDir(outDir).filePath("PDF");
    QDir().mkpath(pdfDir);

    // Derive PDF filename from the first page's source image.
    PageSequence const seq = m_ptrPages->toPageSequence(PAGE_VIEW);
    QString baseName = "output";
    if (seq.numPages() > 0) {
        baseName = QFileInfo(
            seq.pageAt(0).imageId().filePath()
        ).completeBaseName();
    }
    QString const pdfPath = QDir(pdfDir).filePath(baseName + "_vectorized.pdf");

    VectorPdfExporter::Options opts;
    opts.language = m_ocrLanguage;
    opts.dpi = m_pdfDpi;

    int const result = VectorPdfExporter::exportVectorizedPdf(
        m_ptrPages, m_outFileNameGen, pdfPath, opts,
        /*lossless=*/true, /*vectorize=*/true);

    if (result > 0) {
        QMessageBox::information(this, tr("Export"),
            tr("Batch complete.\nVectorized PDF (%1 pages):\n%2")
                .arg(result).arg(pdfPath));
    } else if (result == 0) {
        QMessageBox::warning(this, tr("Export"),
            tr("No processed output images found.\n"
               "Run batch processing first."));
    } else {
        QMessageBox::critical(this, tr("Export"),
            tr("Failed to create PDF.\n"
               "Check that the output path is writable."));
    }
}
```

- [ ] **Step 6: Remove old export methods**

Delete entirely:
- `exportImagesTriggered()`
- `exportBothTriggered()`
- `exportPdfTriggered()`
- `doExportImages()`
- `doExportPdf()`
- `autoGeneratePdfToggled()`
- `jpegQualityChanged()`
- `autoVectorizePdfToggled()`
- `vectorizePdfTriggered()`
- `vectorizePdfStandalone()` (and its continuation)

- [ ] **Step 7: Remove VectorPdfDialog includes and usages**

Remove `#include "VectorPdfDialog.h"` from MainWindow.cpp.
Remove the Strumenti (Tools) menu entry for standalone vectorization if it exists.

- [ ] **Step 8: Build and verify**

```bash
PATH="/c/msys64/mingw64/bin:$PATH"
cmake --build build-qt5 --parallel 2>&1 | tail -30
```

- [ ] **Step 9: Commit**

```bash
git add MainWindow.h MainWindow.cpp
git commit -m "feat: batch completion generates vectorized PDF with Potrace + OCR"
```

---

### Task 7: Delete VectorPdfDialog

**Files:**
- Delete: `VectorPdfDialog.h`
- Delete: `VectorPdfDialog.cpp`
- Modify: `CMakeLists.txt` (remove VectorPdfDialog from source list)

- [ ] **Step 1: Remove VectorPdfDialog source files from CMakeLists.txt**

Find and remove `VectorPdfDialog.cpp` and `VectorPdfDialog.h` from the `SET(common_sources ...)` or equivalent list.

- [ ] **Step 2: Delete the files**

```bash
git rm VectorPdfDialog.h VectorPdfDialog.cpp
```

- [ ] **Step 3: Build and verify**

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt
git commit -m "chore: remove VectorPdfDialog (options are now inline in Export panel)"
```

---

### Task 8: Update CI workflows

**Files:**
- Modify: `.github/workflows/build-windows.yml`
- Modify: `.github/workflows/build-linux.yml`
- Modify: `.github/workflows/build-macos.yml`

- [ ] **Step 1: Windows workflow**

Already has `mingw-w64-x86_64-poppler`. Add `mingw-w64-x86_64-potrace` to the install list.
Add `-DENABLE_POTRACE=ON` to the CMake configure line.

- [ ] **Step 2: Linux workflow**

Add `libpotrace-dev` to the apt install list.
Add `-DENABLE_POTRACE=ON` to CMake configure.

- [ ] **Step 3: macOS workflow**

Add `potrace` to the brew install list.
Add `-DENABLE_POTRACE=ON` to CMake configure.

- [ ] **Step 4: Commit**

```bash
git add .github/workflows/
git commit -m "ci: add Potrace to all CI workflows"
```

---

## Chunk 5: Integration Test

### Task 9: End-to-end manual test

- [ ] **Step 1: Full clean build**

```bash
PATH="/c/msys64/mingw64/bin:$PATH"
cd build-qt5
cmake -S .. -B . -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/mingw64 \
  -DENABLE_POPPLER=ON \
  -DENABLE_TESSERACT=ON \
  -DENABLE_POTRACE=ON
cmake --build . --parallel
```

- [ ] **Step 2: Run ScanTailor and test the pipeline**

1. Open ScanTailor (`build-qt5/scantailor.exe`)
2. Load a PDF (e.g., the Italian text PDF from screenshots)
3. Process through all stages to Output
4. In the Export panel: check "Generate PDF", select Italian + English, set DPI 300
5. Verify size estimate shows
6. Click the arrow button next to "6 Output" to run batch
7. Verify: TIFFs appear in `/out`, and `/out/PDF/<filename>_vectorized.pdf` is created
8. Open the PDF: verify text is vectorized (zoom in — should be smooth curves, not pixels)
9. Try searching text in the PDF viewer — OCR layer should make it searchable

- [ ] **Step 3: Verify PDF file size**

Compare:
- Original PDF size
- Output vectorized PDF size
- The vectorized PDF should be significantly smaller than a raw TIFF-based PDF

- [ ] **Step 4: Final commit with any fixes**

```bash
git add -A
git commit -m "feat: complete Potrace vectorized PDF pipeline"
```
