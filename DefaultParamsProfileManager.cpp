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

#include "DefaultParamsProfileManager.h"
#include "DefaultParams.h"
#include "version.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QDomDocument>
#include <QXmlInputSource>
#include <QXmlSimpleReader>

using namespace page_split;
using namespace output;
using namespace page_layout;

DefaultParamsProfileManager::DefaultParamsProfileManager()
{
	m_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles";
}

DefaultParamsProfileManager::DefaultParamsProfileManager(QString const& path)
:	m_path(path)
{
}

std::list<QString>
DefaultParamsProfileManager::getProfileList() const
{
	std::list<QString> profileList;

	QDir dir(m_path);
	if (dir.exists()) {
		QList<QFileInfo> fileInfoList = dir.entryInfoList();
		for (QFileInfo const& fileInfo : fileInfoList) {
			if (fileInfo.isFile() && ((fileInfo.suffix() == "stp") || (fileInfo.suffix() == "xml"))) {
				profileList.push_back(fileInfo.completeBaseName());
			}
		}
	}
	return profileList;
}

std::unique_ptr<DefaultParams>
DefaultParamsProfileManager::readProfile(QString const& name, LoadStatus* status) const
{
	QDir dir(m_path);
	QFileInfo profile(dir.absoluteFilePath(name + ".stp"));
	if (!profile.exists()) {
		profile = QFileInfo(dir.absoluteFilePath(name + ".xml"));
		if (!profile.exists()) {
			if (status) {
				*status = IO_ERROR;
			}
			return nullptr;
		}
	}

	QFile profileFile(profile.filePath());
	if (!profileFile.open(QIODevice::ReadOnly)) {
		if (status) {
			*status = IO_ERROR;
		}
		return nullptr;
	}

	QDomDocument doc;
	QXmlSimpleReader reader;
	reader.setFeature("http://xml.org/sax/features/external-general-entities", false);
	reader.setFeature("http://xml.org/sax/features/external-parameter-entities", false);
	QXmlInputSource source(&profileFile);
	if (!doc.setContent(&source, &reader)) {
		if (status) {
			*status = IO_ERROR;
		}
		return nullptr;
	}

	profileFile.close();

	QDomElement const profileElement(doc.documentElement());
	QString const version = profileElement.attribute("version");
	if (version.isNull() || (version != VERSION)) {
		if (status) {
			*status = INCOMPATIBLE_VERSION_ERROR;
		}
		return nullptr;
	}

	QDomElement const defaultParamsElement(profileElement.namedItem("default-params").toElement());

	if (status) {
		*status = SUCCESS;
	}
	return std::make_unique<DefaultParams>(defaultParamsElement);
}

bool
DefaultParamsProfileManager::writeProfile(DefaultParams const& params, QString const& name) const
{
	QDomDocument doc;
	QDomElement rootElement(doc.createElement("profile"));
	doc.appendChild(rootElement);
	rootElement.setAttribute("version", VERSION);
	rootElement.appendChild(params.toXml(doc, "default-params"));

	QDir dir(m_path);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	QFile file(dir.absoluteFilePath(name + ".stp"));
	if (file.open(QIODevice::WriteOnly)) {
		QTextStream textStream(&file);
		doc.save(textStream, 2);
		return true;
	}
	return false;
}

bool
DefaultParamsProfileManager::deleteProfile(QString const& name) const
{
	QDir dir(m_path);
	QFileInfo profile(dir.absoluteFilePath(name + ".stp"));
	if (!profile.exists()) {
		profile = QFileInfo(dir.absoluteFilePath(name + ".xml"));
		if (!profile.exists()) {
			return false;
		}
	}

	QFile profileFile(profile.filePath());
	return profileFile.remove();
}

std::unique_ptr<DefaultParams>
DefaultParamsProfileManager::createDefaultProfile() const
{
	return std::make_unique<DefaultParams>();
}

std::unique_ptr<DefaultParams>
DefaultParamsProfileManager::createSourceProfile() const
{
	DefaultParams::DeskewParams deskewParams;
	deskewParams.setMode(MODE_MANUAL);

	DefaultParams::PageSplitParams pageSplitParams;
	pageSplitParams.setLayoutType(SINGLE_PAGE_UNCUT);

	DefaultParams::SelectContentParams selectContentParams;
	selectContentParams.setContentDetectEnabled(false);

	DefaultParams::PageLayoutParams pageLayoutParams;
	pageLayoutParams.setHardMargins(Margins(0, 0, 0, 0));

	Alignment alignment;
	alignment.setNull(true);
	pageLayoutParams.setAlignment(alignment);

	DefaultParams::OutputParams outputParams;

	ColorParams colorParams;
	colorParams.setColorMode(ColorParams::COLOR_GRAYSCALE);
	colorParams.setFillingColor(FILL_BACKGROUND);

	outputParams.setColorParams(colorParams);

	return std::make_unique<DefaultParams>(
		DefaultParams::FixOrientationParams(),
		deskewParams, pageSplitParams,
		selectContentParams, pageLayoutParams,
		outputParams
	);
}
