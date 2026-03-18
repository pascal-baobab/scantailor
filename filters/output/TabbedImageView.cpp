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
#include "TabbedImageView.h"
#include "ImageViewBase.h"
#include <cmath>
#include <algorithm>

namespace output
{

TabbedImageView::TabbedImageView(QWidget* parent)
:	QTabWidget(parent),
	m_prevImageViewTabIndex(0)
{
	connect(this, SIGNAL(currentChanged(int)), SLOT(tabChangedSlot(int)));
}

void
TabbedImageView::addTab(QWidget* widget, QString const& label, ImageViewTab tab)
{
	QTabWidget::addTab(widget, label);
	m_registry[widget] = tab;
}

void
TabbedImageView::setImageRectMap(std::unique_ptr<TabImageRectMap> tabImageRectMap)
{
	m_tabImageRectMap = std::move(tabImageRectMap);
}

void
TabbedImageView::setCurrentTab(ImageViewTab const tab)
{
	int const cnt = count();
	for (int i = 0; i < cnt; ++i) {
		QWidget* wgt = widget(i);
		std::map<QWidget*, ImageViewTab>::const_iterator it(m_registry.find(wgt));
		if (it != m_registry.end()) {
			if (it->second == tab) {
				setCurrentIndex(i);
				break;
			}
		}
	}
}

void
TabbedImageView::tabChangedSlot(int const idx)
{
	QWidget* wgt = widget(idx);
	std::map<QWidget*, ImageViewTab>::const_iterator it(m_registry.find(wgt));
	if (it != m_registry.end()) {
		emit tabChanged(it->second);
	}

	copyViewZoomAndPos(m_prevImageViewTabIndex, idx);

	if (findImageView(widget(idx)) != nullptr) {
		m_prevImageViewTabIndex = idx;
	}
}

ImageViewBase*
TabbedImageView::findImageView(QWidget* widget)
{
	if (!widget) {
		return nullptr;
	}

	if (ImageViewBase* iv = dynamic_cast<ImageViewBase*>(widget)) {
		return iv;
	}

	for (QObject* child : widget->children()) {
		QWidget* childWidget = dynamic_cast<QWidget*>(child);
		if (childWidget) {
			ImageViewBase* iv = findImageView(childWidget);
			if (iv) {
				return iv;
			}
		}
	}
	return nullptr;
}

void
TabbedImageView::copyViewZoomAndPos(int const oldIdx, int const newIdx) const
{
	if (!m_tabImageRectMap.get()) {
		return;
	}

	if (m_registry.find(widget(oldIdx)) == m_registry.end()
	    || m_registry.find(widget(newIdx)) == m_registry.end()) {
		return;
	}

	ImageViewTab const oldViewTab = m_registry.at(widget(oldIdx));
	ImageViewTab const newViewTab = m_registry.at(widget(newIdx));

	if (m_tabImageRectMap->find(oldViewTab) == m_tabImageRectMap->end()
	    || m_tabImageRectMap->find(newViewTab) == m_tabImageRectMap->end()) {
		return;
	}

	QRectF const& oldViewRect = m_tabImageRectMap->at(oldViewTab);
	QRectF const& newViewRect = m_tabImageRectMap->at(newViewTab);

	ImageViewBase* oldImageView = findImageView(widget(oldIdx));
	ImageViewBase* newImageView = findImageView(widget(newIdx));
	if (!oldImageView || !newImageView) {
		return;
	}
	if (oldImageView == newImageView) {
		return;
	}

	if (oldImageView->zoomLevel() != 1.0) {
		QPointF const viewFocus = getFocus(
			oldViewRect,
			*oldImageView->horizontalScrollBar(),
			*oldImageView->verticalScrollBar()
		);
		double const zoomFactor =
			std::max(newViewRect.width(), newViewRect.height())
			/ std::max(oldViewRect.width(), oldViewRect.height());
		newImageView->setZoomLevel(qMax(1.0, oldImageView->zoomLevel() * zoomFactor));
		setFocus(
			*newImageView->horizontalScrollBar(),
			*newImageView->verticalScrollBar(),
			newViewRect, viewFocus
		);
	}
}

QPointF
TabbedImageView::getFocus(
	QRectF const& rect,
	QScrollBar const& horBar,
	QScrollBar const& verBar) const
{
	int const horBarLength = horBar.maximum() - horBar.minimum() + horBar.pageStep();
	int const verBarLength = verBar.maximum() - verBar.minimum() + verBar.pageStep();

	qreal x = ((horBar.value() + (horBar.pageStep() / 2.0)) / horBarLength) * rect.width() + rect.left();
	qreal y = ((verBar.value() + (verBar.pageStep() / 2.0)) / verBarLength) * rect.height() + rect.top();
	return QPointF(x, y);
}

void
TabbedImageView::setFocus(
	QScrollBar& horBar, QScrollBar& verBar,
	QRectF const& rect, QPointF const& focal) const
{
	int const horBarLength = horBar.maximum() - horBar.minimum() + horBar.pageStep();
	int const verBarLength = verBar.maximum() - verBar.minimum() + verBar.pageStep();

	int horValue = (int)std::round(
		((focal.x() - rect.left()) / rect.width()) * horBarLength - (horBar.pageStep() / 2.0)
	);
	int verValue = (int)std::round(
		((focal.y() - rect.top()) / rect.height()) * verBarLength - (verBar.pageStep() / 2.0)
	);

	horValue = qBound(horBar.minimum(), horValue, horBar.maximum());
	verValue = qBound(verBar.minimum(), verValue, verBar.maximum());

	horBar.setValue(horValue);
	verBar.setValue(verValue);
}

} // namespace output
