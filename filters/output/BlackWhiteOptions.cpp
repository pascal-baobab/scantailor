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

#include "BlackWhiteOptions.h"
#include <QDomDocument>
#include <QDomElement>
#include <QString>

namespace output
{

/*============ ColorSegmenterOptions ============*/

BlackWhiteOptions::ColorSegmenterOptions::ColorSegmenterOptions()
:	m_isEnabled(false),
	m_noiseReduction(7),
	m_redThresholdAdjustment(0),
	m_greenThresholdAdjustment(0),
	m_blueThresholdAdjustment(0)
{
}

BlackWhiteOptions::ColorSegmenterOptions::ColorSegmenterOptions(QDomElement const& el)
:	m_isEnabled(el.attribute("enabled", "0").toInt() != 0),
	m_noiseReduction(el.attribute("noiseReduction", "7").toInt()),
	m_redThresholdAdjustment(el.attribute("redAdj", "0").toInt()),
	m_greenThresholdAdjustment(el.attribute("greenAdj", "0").toInt()),
	m_blueThresholdAdjustment(el.attribute("blueAdj", "0").toInt())
{
}

QDomElement
BlackWhiteOptions::ColorSegmenterOptions::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.setAttribute("enabled", (int)m_isEnabled);
	el.setAttribute("noiseReduction", m_noiseReduction);
	el.setAttribute("redAdj", m_redThresholdAdjustment);
	el.setAttribute("greenAdj", m_greenThresholdAdjustment);
	el.setAttribute("blueAdj", m_blueThresholdAdjustment);
	return el;
}

bool
BlackWhiteOptions::ColorSegmenterOptions::operator==(ColorSegmenterOptions const& other) const
{
	return m_isEnabled == other.m_isEnabled
		&& m_noiseReduction == other.m_noiseReduction
		&& m_redThresholdAdjustment == other.m_redThresholdAdjustment
		&& m_greenThresholdAdjustment == other.m_greenThresholdAdjustment
		&& m_blueThresholdAdjustment == other.m_blueThresholdAdjustment;
}

bool
BlackWhiteOptions::ColorSegmenterOptions::operator!=(ColorSegmenterOptions const& other) const
{
	return !(*this == other);
}

/*============ BlackWhiteOptions ============*/

BlackWhiteOptions::BlackWhiteOptions()
:	m_whiteMargins(true),
	m_normalizeIllumination(true),
	m_thresholdAdjustment(0),
	m_binarizationMethod(OTSU),
	m_windowSize(51),
	m_sauvolaCoef(0.34),
	m_wolfLowerBound(1),
	m_wolfUpperBound(254)
{
}

BlackWhiteOptions::BlackWhiteOptions(QDomElement const& el)
:	m_whiteMargins(el.attribute("whiteMargins", "1").toInt() != 0),
	m_normalizeIllumination(el.attribute("normalizeIllumination", "1").toInt() != 0),
	m_thresholdAdjustment(el.attribute("thresholdAdj").toInt()),
	m_binarizationMethod(parseBinarizationMethod(el.attribute("binarizationMethod"))),
	m_colorSegmenterOptions(el.namedItem("colorSegmenter").toElement()),
	m_windowSize(el.attribute("windowSize", "51").toInt()),
	m_sauvolaCoef(el.attribute("sauvolaCoef", "0.34").toDouble()),
	m_wolfLowerBound(el.attribute("wolfLowerBound", "1").toInt()),
	m_wolfUpperBound(el.attribute("wolfUpperBound", "254").toInt())
{
}

QDomElement
BlackWhiteOptions::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.setAttribute("whiteMargins", (int)m_whiteMargins);
	el.setAttribute("normalizeIllumination", (int)m_normalizeIllumination);
	el.setAttribute("thresholdAdj", m_thresholdAdjustment);
	el.setAttribute("binarizationMethod", formatBinarizationMethod(m_binarizationMethod));
	el.setAttribute("windowSize", m_windowSize);
	el.setAttribute("sauvolaCoef", m_sauvolaCoef);
	el.setAttribute("wolfLowerBound", m_wolfLowerBound);
	el.setAttribute("wolfUpperBound", m_wolfUpperBound);
	el.appendChild(m_colorSegmenterOptions.toXml(doc, "colorSegmenter"));
	return el;
}

bool
BlackWhiteOptions::operator==(BlackWhiteOptions const& other) const
{
	return m_whiteMargins == other.m_whiteMargins
		&& m_normalizeIllumination == other.m_normalizeIllumination
		&& m_thresholdAdjustment == other.m_thresholdAdjustment
		&& m_binarizationMethod == other.m_binarizationMethod
		&& m_colorSegmenterOptions == other.m_colorSegmenterOptions
		&& m_windowSize == other.m_windowSize
		&& m_sauvolaCoef == other.m_sauvolaCoef
		&& m_wolfLowerBound == other.m_wolfLowerBound
		&& m_wolfUpperBound == other.m_wolfUpperBound;
}

bool
BlackWhiteOptions::operator!=(BlackWhiteOptions const& other) const
{
	return !(*this == other);
}

BinarizationMethod
BlackWhiteOptions::parseBinarizationMethod(QString const& str)
{
	if (str == "sauvola") return SAUVOLA;
	if (str == "wolf") return WOLF;
	return OTSU;
}

QString
BlackWhiteOptions::formatBinarizationMethod(BinarizationMethod method)
{
	switch (method) {
		case SAUVOLA: return "sauvola";
		case WOLF:    return "wolf";
		default:      return "otsu";
	}
}

} // namespace output
