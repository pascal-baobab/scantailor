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

#ifndef IMAGEPROC_POSTERIZER_H_
#define IMAGEPROC_POSTERIZER_H_

#include <QImage>
#include <unordered_map>

namespace imageproc
{

class Posterizer
{
public:
	explicit Posterizer(int level,
	                    bool normalize = false,
	                    bool forceBlackAndWhite = false,
	                    int normalizeBlackLevel = 0,
	                    int normalizeWhiteLevel = 255);

	static QVector<QRgb> buildPalette(QImage const& image);

	static QImage convertToIndexed(QImage const& image);

	static QImage convertToIndexed(QImage const& image, QVector<QRgb> const& palette);

	QImage posterize(QImage const& image) const;

private:
	int m_level;
	bool m_normalize;
	bool m_forceBlackAndWhite;
	int m_normalizeBlackLevel;
	int m_normalizeWhiteLevel;
};

} // namespace imageproc

#endif // IMAGEPROC_POSTERIZER_H_
