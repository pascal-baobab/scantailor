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

#include "Units.h"
#include <QObject>

QString unitsToString(Units units)
{
	switch (units) {
		case PIXELS:      return "px";
		case MILLIMETRES: return "mm";
		case CENTIMETRES: return "cm";
		case INCHES:      return "in";
	}
	return "mm";
}

Units unitsFromString(const QString& string)
{
	if (string == "px") return PIXELS;
	if (string == "cm") return CENTIMETRES;
	if (string == "in") return INCHES;
	return MILLIMETRES;
}

QString unitsToLocalizedString(Units units)
{
	switch (units) {
		case PIXELS:      return QObject::tr("px");
		case MILLIMETRES: return QObject::tr("mm");
		case CENTIMETRES: return QObject::tr("cm");
		case INCHES:      return QObject::tr("in");
	}
	return QObject::tr("mm");
}
