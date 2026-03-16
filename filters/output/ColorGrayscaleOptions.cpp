/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "ColorGrayscaleOptions.h"
#include <QDomDocument>
#include <QDomElement>
#include <QString>

namespace output
{

/*============ PosterizationOptions ============*/

ColorGrayscaleOptions::PosterizationOptions::PosterizationOptions()
:	m_isEnabled(false),
	m_level(4),
	m_normalizationEnabled(false),
	m_forceBlackAndWhite(true)
{
}

ColorGrayscaleOptions::PosterizationOptions::PosterizationOptions(QDomElement const& el)
:	m_isEnabled(el.attribute("enabled", "0").toInt() != 0),
	m_level(el.attribute("level", "4").toInt()),
	m_normalizationEnabled(el.attribute("normalize", "0").toInt() != 0),
	m_forceBlackAndWhite(el.attribute("forceBlackAndWhite", "1").toInt() != 0)
{
}

QDomElement
ColorGrayscaleOptions::PosterizationOptions::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.setAttribute("enabled", (int)m_isEnabled);
	el.setAttribute("level", m_level);
	el.setAttribute("normalize", (int)m_normalizationEnabled);
	el.setAttribute("forceBlackAndWhite", (int)m_forceBlackAndWhite);
	return el;
}

bool
ColorGrayscaleOptions::PosterizationOptions::operator==(PosterizationOptions const& other) const
{
	return m_isEnabled == other.m_isEnabled
		&& m_level == other.m_level
		&& m_normalizationEnabled == other.m_normalizationEnabled
		&& m_forceBlackAndWhite == other.m_forceBlackAndWhite;
}

bool
ColorGrayscaleOptions::PosterizationOptions::operator!=(PosterizationOptions const& other) const
{
	return !(*this == other);
}

/*============ ColorGrayscaleOptions ============*/

ColorGrayscaleOptions::ColorGrayscaleOptions(QDomElement const& el)
:	m_whiteMargins(el.attribute("whiteMargins") == "1"),
	m_normalizeIllumination(el.attribute("normalizeIllumination") == "1"),
	m_posterizationOptions(el.namedItem("posterization").toElement())
{
}

QDomElement
ColorGrayscaleOptions::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.setAttribute("whiteMargins", m_whiteMargins ? "1" : "0");
	el.setAttribute("normalizeIllumination", m_normalizeIllumination ? "1" : "0");
	el.appendChild(m_posterizationOptions.toXml(doc, "posterization"));
	return el;
}

bool
ColorGrayscaleOptions::operator==(ColorGrayscaleOptions const& other) const
{
	if (m_whiteMargins != other.m_whiteMargins) {
		return false;
	}
	if (m_normalizeIllumination != other.m_normalizeIllumination) {
		return false;
	}
	if (m_posterizationOptions != other.m_posterizationOptions) {
		return false;
	}
	return true;
}

bool
ColorGrayscaleOptions::operator!=(ColorGrayscaleOptions const& other) const
{
	return !(*this == other);
}

} // namespace output
