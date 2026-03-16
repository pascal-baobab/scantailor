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

#include "SplittingOptions.h"
#include <QDomDocument>

namespace output
{

SplittingOptions::SplittingOptions()
:	m_isSplitOutput(false),
	m_splittingMode(BLACK_AND_WHITE_FOREGROUND),
	m_isOriginalBackgroundEnabled(false)
{
}

SplittingOptions::SplittingOptions(QDomElement const& el)
:	m_isSplitOutput(el.attribute("splitOutput") == "1"),
	m_splittingMode(parseSplittingMode(el.attribute("splittingMode"))),
	m_isOriginalBackgroundEnabled(el.attribute("originalBackground") == "1")
{
}

QDomElement SplittingOptions::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.setAttribute("splitOutput", m_isSplitOutput ? "1" : "0");
	el.setAttribute("splittingMode", formatSplittingMode(m_splittingMode));
	el.setAttribute("originalBackground", m_isOriginalBackgroundEnabled ? "1" : "0");
	return el;
}

SplittingMode SplittingOptions::parseSplittingMode(QString const& str)
{
	if (str == "color") {
		return COLOR_FOREGROUND;
	}
	return BLACK_AND_WHITE_FOREGROUND;
}

QString SplittingOptions::formatSplittingMode(SplittingMode mode)
{
	switch (mode) {
		case COLOR_FOREGROUND:
			return "color";
		case BLACK_AND_WHITE_FOREGROUND:
		default:
			return "bw";
	}
}

bool SplittingOptions::operator==(SplittingOptions const& other) const
{
	return (m_isSplitOutput == other.m_isSplitOutput)
		&& (m_splittingMode == other.m_splittingMode)
		&& (m_isOriginalBackgroundEnabled == other.m_isOriginalBackgroundEnabled);
}

bool SplittingOptions::operator!=(SplittingOptions const& other) const
{
	return !(*this == other);
}

} // namespace output
