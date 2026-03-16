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

#ifndef UNITS_PROVIDER_H_
#define UNITS_PROVIDER_H_

#include <list>
#include "UnitsListener.h"

class Dpi;

class UnitsProvider
{
public:
	static UnitsProvider& getInstance();

	Units getUnits() const { return m_units; }

	void setUnits(Units units);

	void addListener(UnitsListener* listener);

	void removeListener(UnitsListener* listener);

	void convertFrom(double& horizontalValue, double& verticalValue,
	                 Units fromUnits, const Dpi& dpi) const;

	void convertTo(double& horizontalValue, double& verticalValue,
	               Units toUnits, const Dpi& dpi) const;

private:
	UnitsProvider();

	void unitsChanged();

	std::list<UnitsListener*> m_unitsListeners;
	Units m_units;
};

#endif // UNITS_PROVIDER_H_
