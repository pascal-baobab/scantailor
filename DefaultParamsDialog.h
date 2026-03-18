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

#ifndef DEFAULTPARAMSDIALOG_H_
#define DEFAULTPARAMSDIALOG_H_

#include <QDialog>
#include "DefaultParams.h"
#include "DefaultParamsProfileManager.h"

class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;

class DefaultParamsDialog : public QDialog
{
	Q_OBJECT
public:
	explicit DefaultParamsDialog(QWidget* parent = nullptr);

private slots:
	void profileChanged(int idx);
	void profileSavePressed();
	void profileDeletePressed();
	void commitChanges();

private:
	void loadProfile(QString const& name);
	void updateUIFromParams(DefaultParams const& params);
	DefaultParams paramsFromUI() const;

	DefaultParamsProfileManager m_profileManager;

	QComboBox* m_profileCB;

	// Page Layout
	QDoubleSpinBox* m_topMarginSB;
	QDoubleSpinBox* m_bottomMarginSB;
	QDoubleSpinBox* m_leftMarginSB;
	QDoubleSpinBox* m_rightMarginSB;

	// Output
	QSpinBox* m_dpiSB;
	QComboBox* m_colorModeCB;

	// Deskew
	QCheckBox* m_deskewAutoCB;
};

#endif
