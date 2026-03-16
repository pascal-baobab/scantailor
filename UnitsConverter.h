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

#ifndef UNITS_CONVERTER_H_
#define UNITS_CONVERTER_H_

#include <QTransform>
#include "Dpi.h"
#include "Units.h"

class UnitsConverter
{
public:
	UnitsConverter() = default;

	explicit UnitsConverter(const Dpi& dpi);

	void convert(double& horizontalValue, double& verticalValue,
	             Units fromUnits, Units toUnits) const;

	QTransform transform(Units fromUnits, Units toUnits) const;

	const Dpi& getDpi() const { return m_dpi; }

	void setDpi(const Dpi& dpi) { m_dpi = dpi; }

private:
	Dpi m_dpi;
};

#endif // UNITS_CONVERTER_H_
