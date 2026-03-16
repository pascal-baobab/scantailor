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

#include "CollapsibleGroupBox.h"
#include "foundation/ScopedIncDec.h"
#include <QSettings>
#include <QEvent>
#include <QShowEvent>
#include <QChildEvent>
#include <QStyle>
#include <QApplication>

CollapsibleGroupBox::CollapsibleGroupBox(QWidget* parent)
:	QGroupBox(parent),
	m_collapsed(false),
	m_shown(false),
	m_collapseButton(0),
	m_ignoreVisibilityEvents(0)
{
	initialize();
}

CollapsibleGroupBox::CollapsibleGroupBox(QString const& title, QWidget* parent)
:	QGroupBox(title, parent),
	m_collapsed(false),
	m_shown(false),
	m_collapseButton(0),
	m_ignoreVisibilityEvents(0)
{
	initialize();
}

void
CollapsibleGroupBox::initialize()
{
	m_collapseButton = new QToolButton(this);
	m_collapseButton->setObjectName("collapseButton");
	m_collapseButton->setAutoRaise(true);
	m_collapseButton->setFixedSize(14, 14);
	m_collapseButton->setIconSize(QSize(10, 10));
	m_collapseButton->setIcon(
		QApplication::style()->standardIcon(QStyle::SP_TitleBarShadeButton)
	);
	setFocusProxy(m_collapseButton);
	setFocusPolicy(Qt::StrongFocus);
	setAlignment(Qt::AlignCenter);

	connect(m_collapseButton, SIGNAL(clicked()), this, SLOT(toggleCollapsed()));
	connect(this, SIGNAL(toggled(bool)), this, SLOT(checkToggled(bool)));
	connect(this, SIGNAL(clicked(bool)), this, SLOT(checkClicked(bool)));
}

void
CollapsibleGroupBox::setCollapsed(bool collapse)
{
	bool const changed = (collapse != m_collapsed);
	if (changed) {
		m_collapsed = collapse;
		m_collapseButton->setIcon(
			QApplication::style()->standardIcon(
				collapse ? QStyle::SP_TitleBarUnshadeButton : QStyle::SP_TitleBarShadeButton
			)
		);
		updateWidgets();
		emit collapsedStateChanged(isCollapsed());
	}
}

bool
CollapsibleGroupBox::isCollapsed() const
{
	return m_collapsed;
}

void
CollapsibleGroupBox::checkToggled(bool)
{
	m_collapseButton->setEnabled(true);
}

void
CollapsibleGroupBox::checkClicked(bool checked)
{
	if (checked && isCollapsed()) {
		setCollapsed(false);
	} else if (!checked && !isCollapsed()) {
		setCollapsed(true);
	}
}

void
CollapsibleGroupBox::toggleCollapsed()
{
	QToolButton* sender = dynamic_cast<QToolButton*>(QObject::sender());
	bool const isSenderCollapseButton = (sender && (sender == m_collapseButton));

	if (isSenderCollapseButton) {
		setCollapsed(!isCollapsed());
	}
}

void
CollapsibleGroupBox::updateWidgets()
{
	ScopedIncDec<int> const guard(m_ignoreVisibilityEvents);

	if (m_collapsed) {
		for (QObject* child : children()) {
			QWidget* widget = dynamic_cast<QWidget*>(child);
			if (widget && (widget != m_collapseButton) && widget->isVisible()) {
				m_collapsedWidgets.insert(widget);
				widget->hide();
			}
		}
	} else {
		for (QObject* child : children()) {
			QWidget* widget = dynamic_cast<QWidget*>(child);
			if (widget && (widget != m_collapseButton)
			    && (m_collapsedWidgets.find(widget) != m_collapsedWidgets.end())) {
				m_collapsedWidgets.erase(widget);
				widget->show();
			}
		}
	}
}

void
CollapsibleGroupBox::showEvent(QShowEvent* event)
{
	if (m_shown) {
		event->accept();
		return;
	}
	m_shown = true;
	loadState();
	QWidget::showEvent(event);
}

void
CollapsibleGroupBox::changeEvent(QEvent* event)
{
	QGroupBox::changeEvent(event);
	if ((event->type() == QEvent::EnabledChange) && isEnabled()) {
		m_collapseButton->setEnabled(true);
	}
}

void
CollapsibleGroupBox::childEvent(QChildEvent* event)
{
	QWidget* childWidget = dynamic_cast<QWidget*>(event->child());
	if (childWidget && (event->type() == QEvent::ChildAdded)) {
		if (m_collapsed) {
			if (childWidget->isVisible()) {
				m_collapsedWidgets.insert(childWidget);
				childWidget->hide();
			}
		}
		childWidget->installEventFilter(this);
	}
	QGroupBox::childEvent(event);
}

bool
CollapsibleGroupBox::eventFilter(QObject* watched, QEvent* event)
{
	if (m_collapsed && !m_ignoreVisibilityEvents) {
		QWidget* childWidget = dynamic_cast<QWidget*>(watched);
		if (childWidget) {
			if (event->type() == QEvent::ShowToParent) {
				ScopedIncDec<int> const guard(m_ignoreVisibilityEvents);
				m_collapsedWidgets.insert(childWidget);
				childWidget->hide();
			} else if (event->type() == QEvent::HideToParent) {
				m_collapsedWidgets.erase(childWidget);
			}
		}
	}
	return QObject::eventFilter(watched, event);
}

CollapsibleGroupBox::~CollapsibleGroupBox()
{
	saveState();
}

void
CollapsibleGroupBox::loadState()
{
	if (!isEnabled()) {
		return;
	}

	QString const key = getSettingsKey();
	if (key.isEmpty()) {
		return;
	}

	setUpdatesEnabled(false);

	QSettings settings;

	if (isCheckable()) {
		QVariant val = settings.value(key + "/checked");
		if (!val.isNull()) {
			setChecked(val.toBool());
		}
	}

	{
		QVariant val = settings.value(key + "/collapsed");
		if (!val.isNull()) {
			setCollapsed(val.toBool());
		}
	}

	setUpdatesEnabled(true);
}

void
CollapsibleGroupBox::saveState()
{
	if (!m_shown || !isEnabled()) {
		return;
	}

	QString const key = getSettingsKey();
	if (key.isEmpty()) {
		return;
	}

	QSettings settings;

	if (isCheckable()) {
		settings.setValue(key + "/checked", isChecked());
	}
	settings.setValue(key + "/collapsed", isCollapsed());
}

QString
CollapsibleGroupBox::getSettingsKey() const
{
	if (objectName().isEmpty()) {
		return QString();
	}

	return "CollapsibleGroupBox/" + objectName();
}
