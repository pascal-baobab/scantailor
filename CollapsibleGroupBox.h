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

#ifndef COLLAPSIBLEGROUPBOX_H_
#define COLLAPSIBLEGROUPBOX_H_

#include <QGroupBox>
#include <QToolButton>
#include <set>

class CollapsibleGroupBox : public QGroupBox
{
	Q_OBJECT
	Q_PROPERTY(bool collapsed READ isCollapsed WRITE setCollapsed USER true)

public:
	explicit CollapsibleGroupBox(QWidget* parent = 0);
	explicit CollapsibleGroupBox(QString const& title, QWidget* parent = 0);
	~CollapsibleGroupBox();

	bool isCollapsed() const;
	void setCollapsed(bool collapse);

signals:
	void collapsedStateChanged(bool collapsed);

public slots:
	void checkToggled(bool);
	void checkClicked(bool checked);
	void toggleCollapsed();

protected:
	void updateWidgets();
	void showEvent(QShowEvent* event);
	void changeEvent(QEvent* event);
	void childEvent(QChildEvent* event);
	bool eventFilter(QObject* watched, QEvent* event);
	void initialize();
	void loadState();
	void saveState();
	QString getSettingsKey() const;

private:
	bool m_collapsed;
	bool m_shown;
	QToolButton* m_collapseButton;
	int m_ignoreVisibilityEvents;
	std::set<QWidget*> m_collapsedWidgets;
};

#endif
