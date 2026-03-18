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

#include "SettingsDialog.h"
#include "OpenGLSupport.h"
#include "UnitsProvider.h"
#include "config.h"
#include <QSettings>
#include <QVariant>
#include <QMessageBox>
#include <tiff.h>

SettingsDialog::SettingsDialog(QWidget* parent)
:	QDialog(parent)
{
	ui.setupUi(this);

	QSettings settings;

#ifndef ENABLE_OPENGL
	ui.use3DAcceleration->setChecked(false);
	ui.use3DAcceleration->setEnabled(false);
	ui.use3DAcceleration->setToolTip(tr("Compiled without OpenGL support."));
#else
	if (!OpenGLSupport::supported()) {
		ui.use3DAcceleration->setChecked(false);
		ui.use3DAcceleration->setEnabled(false);
		ui.use3DAcceleration->setToolTip(tr("Your hardware / driver don't provide the necessary features."));
	} else {
		ui.use3DAcceleration->setChecked(
			settings.value("settings/use_3d_acceleration", false).toBool()
		);
	}
#endif

	// TIFF B&W compression
	ui.tiffCompressionBW->addItem(tr("None"), COMPRESSION_NONE);
	ui.tiffCompressionBW->addItem(tr("LZW"), COMPRESSION_LZW);
	ui.tiffCompressionBW->addItem(tr("Deflate"), COMPRESSION_DEFLATE);
	ui.tiffCompressionBW->addItem(tr("CCITT G4"), COMPRESSION_CCITTFAX4);
	{
		int bwComp = settings.value("settings/tiff_bw_compression", (int)COMPRESSION_CCITTFAX4).toInt();
		int idx = ui.tiffCompressionBW->findData(bwComp);
		if (idx >= 0) ui.tiffCompressionBW->setCurrentIndex(idx);
	}

	// TIFF color compression
	ui.tiffCompressionColor->addItem(tr("None"), COMPRESSION_NONE);
	ui.tiffCompressionColor->addItem(tr("LZW"), COMPRESSION_LZW);
	ui.tiffCompressionColor->addItem(tr("Deflate"), COMPRESSION_DEFLATE);
	ui.tiffCompressionColor->addItem(tr("JPEG"), COMPRESSION_JPEG);
	{
		int colorComp = settings.value("settings/tiff_color_compression", (int)COMPRESSION_LZW).toInt();
		int idx = ui.tiffCompressionColor->findData(colorComp);
		if (idx >= 0) ui.tiffCompressionColor->setCurrentIndex(idx);
	}

	// Measurement units
	ui.unitsComboBox->addItem(tr("Pixels"), "px");
	ui.unitsComboBox->addItem(tr("Millimetres (mm)"), "mm");
	ui.unitsComboBox->addItem(tr("Centimetres (cm)"), "cm");
	ui.unitsComboBox->addItem(tr("Inches (in)"), "in");
	{
		QString cur = unitsToString(UnitsProvider::getInstance().getUnits());
		int idx = ui.unitsComboBox->findData(cur);
		if (idx >= 0) ui.unitsComboBox->setCurrentIndex(idx);
	}

	// Thumbnail quality
	ui.thumbnailQualitySB->setValue(
		settings.value("settings/thumbnail_quality", 200).toInt()
	);

	// Color scheme
	ui.colorSchemeBox->addItem(tr("Dark"), "dark");
	ui.colorSchemeBox->addItem(tr("Light"), "light");
	ui.colorSchemeBox->addItem(tr("Native"), "native");
	{
		QString scheme = settings.value("settings/color_scheme", "native").toString();
		int idx = ui.colorSchemeBox->findData(scheme);
		if (idx >= 0) ui.colorSchemeBox->setCurrentIndex(idx);
	}

	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::commitChanges);
}

SettingsDialog::~SettingsDialog()
{
}

void
SettingsDialog::commitChanges()
{
	QSettings settings;
#ifdef ENABLE_OPENGL
	settings.setValue("settings/use_3d_acceleration", ui.use3DAcceleration->isChecked());
#endif
	settings.setValue("settings/tiff_bw_compression", ui.tiffCompressionBW->currentData().toInt());
	settings.setValue("settings/tiff_color_compression", ui.tiffCompressionColor->currentData().toInt());
	UnitsProvider::getInstance().setUnits(
		unitsFromString(ui.unitsComboBox->currentData().toString())
	);
	settings.setValue("settings/thumbnail_quality", ui.thumbnailQualitySB->value());
	QString const newScheme = ui.colorSchemeBox->currentData().toString();
	QString const oldScheme = settings.value("settings/color_scheme", "native").toString();
	if (newScheme != oldScheme) {
		settings.setValue("settings/color_scheme", newScheme);
		QMessageBox::information(
			this, tr("Color Scheme"),
			tr("The color scheme change will take effect after restarting the application.")
		);
	}
}
