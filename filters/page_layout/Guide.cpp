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

#include "Guide.h"
#include <QDomDocument>
#include <QDomElement>
#include <cmath>

namespace page_layout
{

Guide::Guide()
:	m_orientation(Qt::Horizontal), m_position(0)
{
}

Guide::Guide(Qt::Orientation const orientation, double const position)
:	m_orientation(orientation), m_position(position)
{
}

Guide::Guide(QLineF const& line)
:	m_orientation(lineOrientation(line)),
	m_position((m_orientation == Qt::Horizontal) ? line.y1() : line.x1())
{
}

Guide::Guide(QDomElement const& el)
:	m_orientation(orientationFromString(el.attribute("orientation"))),
	m_position(el.attribute("position").toDouble())
{
}

Qt::Orientation
Guide::lineOrientation(QLineF const& line)
{
	double const angleCos = std::abs(
		(line.p2().x() - line.p1().x()) / line.length()
	);
	return (angleCos > (1.0 / std::sqrt(2.0))) ? Qt::Horizontal : Qt::Vertical;
}

Guide::operator QLineF() const
{
	if (m_orientation == Qt::Horizontal) {
		return QLineF(0, m_position, 1, m_position);
	} else {
		return QLineF(m_position, 0, m_position, 1);
	}
}

QDomElement
Guide::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el = doc.createElement(name);
	el.setAttribute("orientation", orientationToString(m_orientation));
	el.setAttribute("position", QString::number(m_position, 'g', 16));
	return el;
}

QString
Guide::orientationToString(Qt::Orientation const orientation)
{
	return (orientation == Qt::Horizontal) ? "horizontal" : "vertical";
}

Qt::Orientation
Guide::orientationFromString(QString const& str)
{
	return (str == "vertical") ? Qt::Vertical : Qt::Horizontal;
}

} // namespace page_layout
