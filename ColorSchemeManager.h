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

#ifndef COLORSCHEMEMANAGER_H_
#define COLORSCHEMEMANAGER_H_

#include "NonCopyable.h"
#include "ColorScheme.h"
#include <QBrush>
#include <QColor>
#include <memory>

class ColorSchemeManager
{
	DECLARE_NON_COPYABLE(ColorSchemeManager)
private:
	ColorSchemeManager() {}

public:
	static ColorSchemeManager& instance();

	void setColorScheme(ColorScheme const& colorScheme);

	QBrush getColorParam(QString const& colorParam, QBrush const& defaultBrush) const;

	QColor getColorParam(QString const& colorParam, QColor const& defaultColor) const;

private:
	std::auto_ptr<ColorScheme::ColorParams> m_colorParams;
};

#endif
