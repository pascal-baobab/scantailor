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

#ifndef IMAGEPROC_COLOR_SEGMENTER_H_
#define IMAGEPROC_COLOR_SEGMENTER_H_

#include "Dpi.h"
#include <QImage>

namespace imageproc
{

class BinaryImage;
class GrayImage;

class ColorSegmenter
{
public:
	ColorSegmenter(Dpi const& dpi, int noiseThreshold,
	               int redThresholdAdjustment,
	               int greenThresholdAdjustment,
	               int blueThresholdAdjustment);

	ColorSegmenter(Dpi const& dpi, int noiseThreshold);

	QImage segment(BinaryImage const& image, QImage const& colorImage) const;

	GrayImage segment(BinaryImage const& image, GrayImage const& grayImage) const;

private:
	Dpi m_dpi;
	int m_noiseThreshold;
	int m_redThresholdAdjustment;
	int m_greenThresholdAdjustment;
	int m_blueThresholdAdjustment;
};

} // namespace imageproc

#endif // IMAGEPROC_COLOR_SEGMENTER_H_
