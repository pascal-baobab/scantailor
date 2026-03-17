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

	// Convert QImage to Potrace bitmap.
	// Potrace expects: 1 = foreground (black), 0 = background (white).
	// Bits packed into potrace_word, MSB = leftmost pixel.
	int const bitsPerWord = static_cast<int>(sizeof(potrace_word) * 8);
	int const wordsPerLine = (w + bitsPerWord - 1) / bitsPerWord;
	int const bitmapSize = wordsPerLine * h;

	potrace_word* bitmapData = static_cast<potrace_word*>(
		std::calloc(bitmapSize, sizeof(potrace_word)));
	if (!bitmapData) {
		return result;
	}

	// Threshold to binary and pack.
	QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
	for (int y = 0; y < h; ++y) {
		uchar const* line = gray.constScanLine(y);
		potrace_word* dst = bitmapData + y * wordsPerLine;
		for (int x = 0; x < w; ++x) {
			if (line[x] < static_cast<uchar>(opts.threshold)) {
				int const wordIdx = x / bitsPerWord;
				int const bitIdx = bitsPerWord - 1 - (x % bitsPerWord);
				dst[wordIdx] |= (static_cast<potrace_word>(1) << bitIdx);
			}
		}
	}

	// Set up Potrace bitmap struct.
	potrace_bitmap_t bm;
	bm.w = w;
	bm.h = h;
	bm.dy = wordsPerLine;
	bm.map = bitmapData;

	// Configure tracing parameters.
	potrace_param_t* param = potrace_param_default();
	if (!param) {
		std::free(bitmapData);
		return result;
	}
	param->turdsize = opts.turdSize;
	param->alphamax = opts.alphaMax;

	// Trace.
	potrace_state_t* state = potrace_trace(param, &bm);
	potrace_param_free(param);
	std::free(bitmapData);

	if (!state || state->status != POTRACE_STATUS_OK) {
		if (state) potrace_state_free(state);
		return result;
	}

	// Convert traced paths to PDF content stream.
	// Potrace pixel (0,0) = top-left; PDF (0,0) = bottom-left.
	double const scaleX = pageW / w;
	double const scaleY = pageH / h;

	result += "q\n";
	result += "0 0 0 rg\n";  // Black fill for text

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
		double sy = pageH - start.y * scaleY;

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
				// POTRACE_CORNER: two straight line segments.
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

		// Close and fill with even-odd rule (handles holes correctly).
		result += "h f*\n";

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
