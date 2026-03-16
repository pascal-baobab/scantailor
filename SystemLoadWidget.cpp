/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) Joseph Artsimovich <joseph.artsimovich@gmail.com>
    Copyright (C) 2019 4lex4 <4lex49@zoho.com>

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

#include "SystemLoadWidget.h"
#include <QSettings>
#include <QThread>
#include <QToolTip>
#include <algorithm>

static const char* const key = "settings/batch_processing_threads";

SystemLoadWidget::SystemLoadWidget(QWidget* parent)
:	QWidget(parent), m_maxThreads(QThread::idealThreadCount())
{
	ui.setupUi(this);

	if (sizeof(void*) <= 4) {
		// Restrict to 2 threads on 32-bit due to address space constraints.
		m_maxThreads = std::min(m_maxThreads, 2);
	}

	int numThreads = std::min(m_maxThreads, QSettings().value(key, m_maxThreads).toInt());

	ui.slider->setRange(1, m_maxThreads);
	ui.slider->setValue(numThreads);

	connect(ui.slider, SIGNAL(sliderPressed()), SLOT(sliderPressed()));
	connect(ui.slider, SIGNAL(sliderMoved(int)), SLOT(sliderMoved(int)));
	connect(ui.slider, SIGNAL(valueChanged(int)), SLOT(valueChanged(int)));
	connect(ui.minusBtn, SIGNAL(clicked()), SLOT(decreaseLoad()));
	connect(ui.plusBtn, SIGNAL(clicked()), SLOT(increaseLoad()));
}

void
SystemLoadWidget::sliderPressed()
{
	showHideToolTip(ui.slider->value());
}

void
SystemLoadWidget::sliderMoved(int threads)
{
	showHideToolTip(threads);
}

void
SystemLoadWidget::valueChanged(int threads)
{
	QSettings settings;
	if (threads == m_maxThreads) {
		settings.remove(key);
	} else {
		settings.setValue(key, threads);
	}
}

void
SystemLoadWidget::decreaseLoad()
{
	ui.slider->setValue(ui.slider->value() - 1);
	showHideToolTip(ui.slider->value());
}

void
SystemLoadWidget::increaseLoad()
{
	ui.slider->setValue(ui.slider->value() + 1);
	showHideToolTip(ui.slider->value());
}

void
SystemLoadWidget::showHideToolTip(int threads)
{
	QPoint const center(ui.slider->rect().center());
	QPoint tooltipPos(ui.slider->mapFromGlobal(QCursor::pos()));
	if (tooltipPos.x() < 0 || tooltipPos.x() >= ui.slider->width()) {
		tooltipPos.setX(center.x());
	}
	if (tooltipPos.y() < 0 || tooltipPos.y() >= ui.slider->height()) {
		tooltipPos.setY(center.y());
	}
	tooltipPos = ui.slider->mapToGlobal(tooltipPos);
	QToolTip::showText(
		tooltipPos,
		QString("%1/%2").arg(threads).arg(m_maxThreads),
		ui.slider
	);
}
