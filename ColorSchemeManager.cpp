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

#include "ColorSchemeManager.h"
#include <QApplication>

ColorSchemeManager&
ColorSchemeManager::instance()
{
	static ColorSchemeManager inst;
	return inst;
}

void
ColorSchemeManager::setColorScheme(ColorScheme const& colorScheme)
{
	if (QStyle* style = colorScheme.getStyle()) {
		qApp->setStyle(style);
	}
	if (QPalette const* palette = colorScheme.getPalette()) {
		qApp->setPalette(*palette);
	}
	if (QString const* styleSheet = colorScheme.getStyleSheet()) {
		qApp->setStyleSheet(*styleSheet);
	}
	if (ColorScheme::ColorParams const* colorParams = colorScheme.getColorParams()) {
		m_colorParams.reset(new ColorScheme::ColorParams(*colorParams));
	}
}

QBrush
ColorSchemeManager::getColorParam(QString const& colorParam, QBrush const& defaultBrush) const
{
	if (!m_colorParams.get()) {
		return defaultBrush;
	}

	ColorScheme::ColorParams::const_iterator it = m_colorParams->find(colorParam);
	if (it != m_colorParams->end()) {
		return QBrush(it->second, defaultBrush.style());
	}
	return defaultBrush;
}

QColor
ColorSchemeManager::getColorParam(QString const& colorParam, QColor const& defaultColor) const
{
	if (!m_colorParams.get()) {
		return defaultColor;
	}

	ColorScheme::ColorParams::const_iterator it = m_colorParams->find(colorParam);
	if (it != m_colorParams->end()) {
		return it->second;
	}
	return defaultColor;
}
