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

#ifndef PAGE_LAYOUT_GUIDE_H_
#define PAGE_LAYOUT_GUIDE_H_

#include <QLineF>
#include <QString>
#include <Qt>

class QDomDocument;
class QDomElement;

namespace page_layout
{

/**
 * A guide line (horizontal or vertical) used as a reference in the margins view.
 * Position is relative to the center of m_outerRect, in virtual image pixels.
 */
class Guide
{
public:
	Guide();

	Guide(Qt::Orientation orientation, double position);

	explicit Guide(QLineF const& line);

	explicit Guide(QDomElement const& el);

	QDomElement toXml(QDomDocument& doc, QString const& name) const;

	operator QLineF() const;

	Qt::Orientation getOrientation() const { return m_orientation; }

	double getPosition() const { return m_position; }

	void setPosition(double position) { m_position = position; }

private:
	static Qt::Orientation lineOrientation(QLineF const& line);

	static QString orientationToString(Qt::Orientation orientation);

	static Qt::Orientation orientationFromString(QString const& str);

	Qt::Orientation m_orientation;
	double m_position;
};

} // namespace page_layout

#endif
