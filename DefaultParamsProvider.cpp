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

#include "DefaultParamsProvider.h"
#include "DefaultParams.h"
#include "DefaultParamsProfileManager.h"
#include <QSettings>
#include <cassert>

DefaultParamsProvider::DefaultParamsProvider()
{
	QSettings settings;
	DefaultParamsProfileManager profileManager;

	QString const profile = settings.value("settings/current_profile", "Default").toString();
	if (profile == "Default") {
		m_params = profileManager.createDefaultProfile();
		m_profileName = profile;
	} else if (profile == "Source") {
		m_params = profileManager.createSourceProfile();
		m_profileName = profile;
	} else {
		std::unique_ptr<DefaultParams> params = profileManager.readProfile(profile);
		if (params.get() != nullptr) {
			m_params = std::move(params);
			m_profileName = profile;
		} else {
			m_params = profileManager.createDefaultProfile();
			m_profileName = "Default";
			settings.setValue("settings/current_profile", "Default");
		}
	}
}

DefaultParamsProvider&
DefaultParamsProvider::getInstance()
{
	static DefaultParamsProvider instance;
	return instance;
}

QString const&
DefaultParamsProvider::getProfileName() const
{
	return m_profileName;
}

DefaultParams const&
DefaultParamsProvider::getParams() const
{
	assert(m_params.get() != nullptr);
	return *m_params;
}

void
DefaultParamsProvider::setParams(std::unique_ptr<DefaultParams> params, QString const& name)
{
	if (params.get() == nullptr) {
		return;
	}

	m_params = std::move(params);
	m_profileName = name;
}
