/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>

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

#include "ImageCombination.h"
#include "BinaryImage.h"
#include <QImage>
#include <stdexcept>
#include <stdint.h>

namespace imageproc
{

namespace
{

template<typename Pixel>
void applyMaskImpl(QImage& image, BinaryImage const& bwMask, BWColor fillingColor)
{
	Pixel* imageLine = reinterpret_cast<Pixel*>(image.bits());
	int const imageStride = image.bytesPerLine() / sizeof(Pixel);
	uint32_t const* bwMaskLine = bwMask.data();
	int const bwMaskStride = bwMask.wordsPerLine();
	int const width = image.width();
	int const height = image.height();
	uint32_t const msb = uint32_t(1) << 31;
	Pixel const fillingPixel = static_cast<Pixel>((fillingColor == WHITE) ? 0xffffffffu : 0x00000000u);

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (!(bwMaskLine[x >> 5] & (msb >> (x & 31)))) {
				imageLine[x] = fillingPixel;
			}
		}
		imageLine += imageStride;
		bwMaskLine += bwMaskStride;
	}
}

} // namespace

void applyMask(QImage& image, BinaryImage const& bwMask, BWColor fillingColor)
{
	if (image.size() != bwMask.size()) {
		throw std::invalid_argument("applyMask: image and mask sizes don't match");
	}

	switch (image.format()) {
		case QImage::Format_Indexed8:
			applyMaskImpl<uint8_t>(image, bwMask, fillingColor);
			break;
		case QImage::Format_RGB32:
		case QImage::Format_ARGB32:
			applyMaskImpl<uint32_t>(image, bwMask, fillingColor);
			break;
		default:
			throw std::invalid_argument("applyMask: unsupported image format");
	}
}

} // namespace imageproc
