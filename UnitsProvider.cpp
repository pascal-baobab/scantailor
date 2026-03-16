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

#include "UnitsProvider.h"
#include "UnitsConverter.h"
#include <QSettings>
#include <algorithm>

UnitsProvider::UnitsProvider()
:	m_units(unitsFromString(QSettings().value("settings/units", "mm").toString()))
{
}

UnitsProvider& UnitsProvider::getInstance()
{
	static UnitsProvider instance;
	return instance;
}

void UnitsProvider::setUnits(Units units)
{
	m_units = units;
	QSettings().setValue("settings/units", unitsToString(units));
	unitsChanged();
}

void UnitsProvider::addListener(UnitsListener* listener)
{
	m_unitsListeners.push_back(listener);
}

void UnitsProvider::removeListener(UnitsListener* listener)
{
	m_unitsListeners.remove(listener);
}

void UnitsProvider::unitsChanged()
{
	for (UnitsListener* listener : m_unitsListeners) {
		listener->onUnitsChanged(m_units);
	}
}

void UnitsProvider::convertFrom(double& horizontalValue, double& verticalValue,
                                Units fromUnits, const Dpi& dpi) const
{
	UnitsConverter(dpi).convert(horizontalValue, verticalValue, fromUnits, m_units);
}

void UnitsProvider::convertTo(double& horizontalValue, double& verticalValue,
                              Units toUnits, const Dpi& dpi) const
{
	UnitsConverter(dpi).convert(horizontalValue, verticalValue, m_units, toUnits);
}
