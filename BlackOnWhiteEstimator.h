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

#ifndef BLACK_ON_WHITE_ESTIMATOR_H_
#define BLACK_ON_WHITE_ESTIMATOR_H_

#include "imageproc/BinaryImage.h"
#include "ImageTransformation.h"
#include <QPolygonF>

class TaskStatus;
class DebugImages;

namespace imageproc {
class GrayImage;
}

class BlackOnWhiteEstimator
{
public:
	static bool isBlackOnWhite(
		imageproc::GrayImage const& grayImage,
		ImageTransformation const& xform,
		TaskStatus const& status,
		DebugImages* dbg = nullptr);

	static bool isBlackOnWhiteRefining(
		imageproc::GrayImage const& grayImage,
		ImageTransformation const& xform,
		TaskStatus const& status,
		DebugImages* dbg = nullptr);

	static bool isBlackOnWhite(
		imageproc::GrayImage const& img,
		imageproc::BinaryImage const& mask);

	static bool isBlackOnWhite(
		imageproc::GrayImage const& img,
		QPolygonF const& cropArea);
};

#endif // BLACK_ON_WHITE_ESTIMATOR_H_
