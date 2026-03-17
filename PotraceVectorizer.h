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
	 * fragment containing the path drawing operators (m, l, c, h, f*).
	 *
	 * @param image     Input image (any format, will be thresholded to B&W)
	 * @param pageW     PDF page width in points (for coordinate scaling)
	 * @param pageH     PDF page height in points (for coordinate scaling)
	 * @param dpi       Image resolution (pixels per inch)
	 * @param opts      Tracing options
	 * @return PDF content stream bytes, empty if tracing fails or HAVE_POTRACE not defined
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
