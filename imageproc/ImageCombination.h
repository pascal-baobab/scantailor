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

#ifndef IMAGEPROC_IMAGE_COMBINATION_H_
#define IMAGEPROC_IMAGE_COMBINATION_H_

#include "BWColor.h"

class QImage;

namespace imageproc
{

class BinaryImage;

/**
 * \brief Fills pixels outside the binary mask with the specified color.
 *
 * Pixels corresponding to BLACK pixels in bwMask are filled with fillingColor.
 * Pixels corresponding to WHITE pixels in bwMask are left unchanged.
 */
void applyMask(QImage& image, BinaryImage const& bwMask, BWColor fillingColor = WHITE);

} // namespace imageproc

#endif // IMAGEPROC_IMAGE_COMBINATION_H_
