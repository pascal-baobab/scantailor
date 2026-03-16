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

#ifndef OUTPUT_TABBED_IMAGE_VIEW_H_
#define OUTPUT_TABBED_IMAGE_VIEW_H_

#include "ImageViewTab.h"
#include <QTabWidget>
#include <QScrollBar>
#include <QPointF>
#include <QRectF>
#include <map>
#include <memory>

class ImageViewBase;

namespace output
{

class TabbedImageView : public QTabWidget
{
	Q_OBJECT
public:
	typedef std::map<ImageViewTab, QRectF> TabImageRectMap;

	TabbedImageView(QWidget* parent = 0);

	void addTab(QWidget* widget, QString const& label, ImageViewTab tab);

	void setImageRectMap(std::auto_ptr<TabImageRectMap> tabImageRectMap);

public slots:
	void setCurrentTab(ImageViewTab tab);
signals:
	void tabChanged(ImageViewTab tab);
private slots:
	void tabChangedSlot(int idx);
private:
	void copyViewZoomAndPos(int oldIdx, int newIdx) const;
	QPointF getFocus(QRectF const& rect, QScrollBar const& horBar, QScrollBar const& verBar) const;
	void setFocus(QScrollBar& horBar, QScrollBar& verBar, QRectF const& rect, QPointF const& focal) const;

	static ImageViewBase* findImageView(QWidget* widget);

	std::map<QWidget*, ImageViewTab> m_registry;
	std::auto_ptr<TabImageRectMap> m_tabImageRectMap;
	int m_prevImageViewTabIndex;
};

} // namespace output

#endif
