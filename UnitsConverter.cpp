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

#include "UnitsConverter.h"
#include "Dpm.h"

UnitsConverter::UnitsConverter(const Dpi& dpi)
:	m_dpi(dpi)
{
}

void UnitsConverter::convert(double& horizontalValue, double& verticalValue,
                             Units fromUnits, Units toUnits) const
{
	if (m_dpi.isNull() || (fromUnits == toUnits)) {
		return;
	}

	Dpm dpm(m_dpi);
	switch (fromUnits) {
		case PIXELS:
			switch (toUnits) {
				case MILLIMETRES:
					horizontalValue = horizontalValue / dpm.horizontal() * 1000.;
					verticalValue   = verticalValue   / dpm.vertical()   * 1000.;
					break;
				case CENTIMETRES:
					horizontalValue = horizontalValue / dpm.horizontal() * 100.;
					verticalValue   = verticalValue   / dpm.vertical()   * 100.;
					break;
				case INCHES:
					horizontalValue /= m_dpi.horizontal();
					verticalValue   /= m_dpi.vertical();
					break;
				default: break;
			}
			break;
		case MILLIMETRES:
			switch (toUnits) {
				case PIXELS:
					horizontalValue = horizontalValue / 1000. * dpm.horizontal();
					verticalValue   = verticalValue   / 1000. * dpm.vertical();
					break;
				case CENTIMETRES:
					horizontalValue /= 10.;
					verticalValue   /= 10.;
					break;
				case INCHES:
					horizontalValue = horizontalValue / 1000. * dpm.horizontal() / m_dpi.horizontal();
					verticalValue   = verticalValue   / 1000. * dpm.vertical()   / m_dpi.vertical();
					break;
				default: break;
			}
			break;
		case CENTIMETRES:
			switch (toUnits) {
				case PIXELS:
					horizontalValue = horizontalValue / 100. * dpm.horizontal();
					verticalValue   = verticalValue   / 100. * dpm.vertical();
					break;
				case MILLIMETRES:
					horizontalValue *= 10.;
					verticalValue   *= 10.;
					break;
				case INCHES:
					horizontalValue = horizontalValue / 100. * dpm.horizontal() / m_dpi.horizontal();
					verticalValue   = verticalValue   / 100. * dpm.vertical()   / m_dpi.vertical();
					break;
				default: break;
			}
			break;
		case INCHES:
			switch (toUnits) {
				case PIXELS:
					horizontalValue *= m_dpi.horizontal();
					verticalValue   *= m_dpi.vertical();
					break;
				case MILLIMETRES:
					horizontalValue = horizontalValue * m_dpi.horizontal() / dpm.horizontal() * 1000.;
					verticalValue   = verticalValue   * m_dpi.vertical()   / dpm.vertical()   * 1000.;
					break;
				case CENTIMETRES:
					horizontalValue = horizontalValue * m_dpi.horizontal() / dpm.horizontal() * 100.;
					verticalValue   = verticalValue   * m_dpi.vertical()   / dpm.vertical()   * 100.;
					break;
				default: break;
			}
			break;
	}
}

QTransform UnitsConverter::transform(Units fromUnits, Units toUnits) const
{
	double xScale = 1.0;
	double yScale = 1.0;
	convert(xScale, yScale, fromUnits, toUnits);
	return QTransform().scale(xScale, yScale);
}
