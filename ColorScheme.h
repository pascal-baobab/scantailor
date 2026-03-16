/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef COLORSCHEME_H_
#define COLORSCHEME_H_

#include <QColor>
#include <QString>
#include <map>

class QStyle;
class QPalette;

class ColorScheme
{
public:
	virtual ~ColorScheme() {}

	virtual QStyle* getStyle() const = 0;

	virtual QPalette const* getPalette() const = 0;

	virtual QString const* getStyleSheet() const = 0;

	typedef std::map<QString, QColor> ColorParams;

	virtual ColorParams const* getColorParams() const = 0;
};

#endif
