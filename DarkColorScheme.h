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

#ifndef DARKCOLORSCHEME_H_
#define DARKCOLORSCHEME_H_

#include "ColorScheme.h"
#include <QPalette>
#include <QString>

class DarkColorScheme : public ColorScheme
{
public:
	DarkColorScheme();

	QStyle* getStyle() const;

	QPalette const* getPalette() const;

	QString const* getStyleSheet() const;

	ColorParams const* getColorParams() const;

private:
	void loadPalette();
	void loadStyleSheet();
	void loadColorParams();

	QPalette m_palette;
	QString m_styleSheet;
	ColorParams m_customColors;
};

#endif
