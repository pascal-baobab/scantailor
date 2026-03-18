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

#include "DefaultParamsDialog.h"
#include "DefaultParamsProvider.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <list>
#include <memory>

DefaultParamsDialog::DefaultParamsDialog(QWidget* parent)
:	QDialog(parent),
	m_profileCB(new QComboBox(this)),
	m_topMarginSB(new QDoubleSpinBox(this)),
	m_bottomMarginSB(new QDoubleSpinBox(this)),
	m_leftMarginSB(new QDoubleSpinBox(this)),
	m_rightMarginSB(new QDoubleSpinBox(this)),
	m_dpiSB(new QSpinBox(this)),
	m_colorModeCB(new QComboBox(this)),
	m_deskewAutoCB(new QCheckBox(tr("Auto"), this))
{
	setWindowTitle(tr("Default Parameters"));
	resize(450, 400);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	// --- Profile selector ---
	QHBoxLayout* profileLayout = new QHBoxLayout;
	profileLayout->addWidget(new QLabel(tr("Profile:"), this));
	profileLayout->addWidget(m_profileCB, 1);

	QPushButton* saveBtn = new QPushButton(tr("Save"), this);
	QPushButton* deleteBtn = new QPushButton(tr("Delete"), this);
	profileLayout->addWidget(saveBtn);
	profileLayout->addWidget(deleteBtn);
	mainLayout->addLayout(profileLayout);

	// --- Tab widget ---
	QTabWidget* tabs = new QTabWidget(this);
	mainLayout->addWidget(tabs, 1);

	// == Deskew tab ==
	{
		QWidget* page = new QWidget;
		QFormLayout* form = new QFormLayout(page);
		form->addRow(tr("Mode:"), m_deskewAutoCB);
		m_deskewAutoCB->setChecked(true);
		tabs->addTab(page, tr("Deskew"));
	}

	// == Page Layout tab ==
	{
		QWidget* page = new QWidget;
		QFormLayout* form = new QFormLayout(page);

		QDoubleSpinBox* spinboxes[] = { m_topMarginSB, m_bottomMarginSB, m_leftMarginSB, m_rightMarginSB };
		for (int i = 0; i < 4; ++i) {
			spinboxes[i]->setRange(0.0, 999.0);
			spinboxes[i]->setDecimals(1);
			spinboxes[i]->setSingleStep(0.5);
			spinboxes[i]->setSuffix(" mm");
		}

		form->addRow(tr("Top margin:"), m_topMarginSB);
		form->addRow(tr("Bottom margin:"), m_bottomMarginSB);
		form->addRow(tr("Left margin:"), m_leftMarginSB);
		form->addRow(tr("Right margin:"), m_rightMarginSB);
		tabs->addTab(page, tr("Page Layout"));
	}

	// == Output tab ==
	{
		QWidget* page = new QWidget;
		QFormLayout* form = new QFormLayout(page);

		m_dpiSB->setRange(50, 2400);
		m_dpiSB->setSingleStep(50);
		m_dpiSB->setValue(600);
		form->addRow(tr("Output DPI:"), m_dpiSB);

		m_colorModeCB->addItem(tr("Black and White"), 0);
		m_colorModeCB->addItem(tr("Color / Grayscale"), 1);
		m_colorModeCB->addItem(tr("Mixed"), 2);
		form->addRow(tr("Color mode:"), m_colorModeCB);
		tabs->addTab(page, tr("Output"));
	}

	// --- Button box ---
	QDialogButtonBox* buttonBox = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this
	);
	mainLayout->addWidget(buttonBox);

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(commitChanges()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	connect(m_profileCB, SIGNAL(currentIndexChanged(int)), this, SLOT(profileChanged(int)));
	connect(saveBtn, SIGNAL(clicked()), this, SLOT(profileSavePressed()));
	connect(deleteBtn, SIGNAL(clicked()), this, SLOT(profileDeletePressed()));

	// Populate profile list
	m_profileCB->blockSignals(true);
	m_profileCB->addItem(tr("Default"));
	m_profileCB->addItem(tr("Source"));

	std::list<QString> profiles = m_profileManager.getProfileList();
	for (std::list<QString>::const_iterator it = profiles.begin(); it != profiles.end(); ++it) {
		m_profileCB->addItem(*it);
	}

	// Select current profile
	QString const currentProfile = DefaultParamsProvider::getInstance().getProfileName();
	int idx = m_profileCB->findText(currentProfile);
	if (idx >= 0) {
		m_profileCB->setCurrentIndex(idx);
	}
	m_profileCB->blockSignals(false);

	loadProfile(m_profileCB->currentText());
}


void
DefaultParamsDialog::profileChanged(int idx)
{
	if (idx < 0) return;
	loadProfile(m_profileCB->currentText());
}


void
DefaultParamsDialog::profileSavePressed()
{
	bool ok = false;
	QString name = QInputDialog::getText(
		this, tr("Save Profile"),
		tr("Profile name:"),
		QLineEdit::Normal, m_profileCB->currentText(), &ok
	);

	if (!ok || name.isEmpty()) return;
	if (name == tr("Default") || name == tr("Source")) {
		QMessageBox::warning(this, tr("Error"),
			tr("Cannot overwrite a built-in profile."));
		return;
	}

	DefaultParams params = paramsFromUI();
	if (!m_profileManager.writeProfile(params, name)) {
		QMessageBox::warning(this, tr("Error"),
			tr("Failed to save the profile."));
		return;
	}

	// Add to combo if new
	if (m_profileCB->findText(name) < 0) {
		m_profileCB->addItem(name);
	}
	m_profileCB->setCurrentIndex(m_profileCB->findText(name));
}


void
DefaultParamsDialog::profileDeletePressed()
{
	QString name = m_profileCB->currentText();
	if (name == tr("Default") || name == tr("Source")) {
		QMessageBox::warning(this, tr("Error"),
			tr("Cannot delete a built-in profile."));
		return;
	}

	if (QMessageBox::question(this, tr("Delete Profile"),
			tr("Delete profile \"%1\"?").arg(name),
			QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
		return;
	}

	m_profileManager.deleteProfile(name);
	int idx = m_profileCB->findText(name);
	if (idx >= 0) {
		m_profileCB->removeItem(idx);
	}
}


void
DefaultParamsDialog::commitChanges()
{
	QString const name = m_profileCB->currentText();
	DefaultParams params = paramsFromUI();

	DefaultParamsProvider::getInstance().setParams(
		std::auto_ptr<DefaultParams>(new DefaultParams(params)), name
	);

	QSettings settings;
	settings.setValue("settings/current_profile", name);

	accept();
}


void
DefaultParamsDialog::loadProfile(QString const& name)
{
	std::auto_ptr<DefaultParams> params;

	if (name == tr("Default")) {
		params = m_profileManager.createDefaultProfile();
	} else if (name == tr("Source")) {
		params = m_profileManager.createSourceProfile();
	} else {
		params = m_profileManager.readProfile(name);
		if (params.get() == nullptr) {
			params = m_profileManager.createDefaultProfile();
		}
	}

	updateUIFromParams(*params);
}


void
DefaultParamsDialog::updateUIFromParams(DefaultParams const& params)
{
	// Deskew
	DefaultParams::DeskewParams const& deskew = params.getDeskewParams();
	m_deskewAutoCB->setChecked(deskew.getMode() == MODE_AUTO);

	// Page Layout margins
	DefaultParams::PageLayoutParams const& pl = params.getPageLayoutParams();
	Margins const& m = pl.getHardMargins();
	m_topMarginSB->setValue(m.top());
	m_bottomMarginSB->setValue(m.bottom());
	m_leftMarginSB->setValue(m.left());
	m_rightMarginSB->setValue(m.right());

	// Output
	DefaultParams::OutputParams const& out = params.getOutputParams();
	m_dpiSB->setValue(out.getDpi().horizontal());

	int colorIdx = 0;
	switch (out.getColorParams().colorMode()) {
		case output::ColorParams::BLACK_AND_WHITE: colorIdx = 0; break;
		case output::ColorParams::COLOR_GRAYSCALE: colorIdx = 1; break;
		case output::ColorParams::MIXED: colorIdx = 2; break;
	}
	m_colorModeCB->setCurrentIndex(colorIdx);
}


DefaultParams
DefaultParamsDialog::paramsFromUI() const
{
	// Deskew
	DefaultParams::DeskewParams deskewParams;
	deskewParams.setMode(m_deskewAutoCB->isChecked() ? MODE_AUTO : MODE_MANUAL);

	// Page Layout
	Margins margins(
		m_leftMarginSB->value(),
		m_topMarginSB->value(),
		m_rightMarginSB->value(),
		m_bottomMarginSB->value()
	);
	DefaultParams::PageLayoutParams pageLayoutParams;
	pageLayoutParams.setHardMargins(margins);

	// Output
	output::ColorParams colorParams;
	switch (m_colorModeCB->currentIndex()) {
		case 0: colorParams.setColorMode(output::ColorParams::BLACK_AND_WHITE); break;
		case 1: colorParams.setColorMode(output::ColorParams::COLOR_GRAYSCALE); break;
		case 2: colorParams.setColorMode(output::ColorParams::MIXED); break;
	}

	DefaultParams::OutputParams outputParams;
	int dpi = m_dpiSB->value();
	outputParams.setDpi(Dpi(dpi, dpi));
	outputParams.setColorParams(colorParams);

	return DefaultParams(
		DefaultParams::FixOrientationParams(),
		deskewParams,
		DefaultParams::PageSplitParams(),
		DefaultParams::SelectContentParams(),
		pageLayoutParams,
		outputParams
	);
}
