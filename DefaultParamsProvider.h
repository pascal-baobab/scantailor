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

#ifndef DEFAULTPARAMSPROVIDER_H_
#define DEFAULTPARAMSPROVIDER_H_

#include "NonCopyable.h"
#include <QString>
#include <memory>

class DefaultParams;

class DefaultParamsProvider
{
	DECLARE_NON_COPYABLE(DefaultParamsProvider)
private:
	DefaultParamsProvider();

public:
	static DefaultParamsProvider& getInstance();

	QString const& getProfileName() const;

	DefaultParams const& getParams() const;

	void setParams(std::unique_ptr<DefaultParams> params, QString const& name);

private:
	QString m_profileName;
	std::unique_ptr<DefaultParams> m_params;
};

#endif
