/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef SELECT_CONTENT_IMAGEVIEW_H_
#define SELECT_CONTENT_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include "DragHandler.h"
#include "ZoomHandler.h"
#include "DraggablePoint.h"
#include "DraggableLineSegment.h"
#include "ObjectDragHandler.h"
#include "InteractionState.h"
#include <QRectF>
#include <QSizeF>
#include <QString>

class ImageTransformation;
class InteractionState;
class QMenu;
class QMouseEvent;

namespace select_content
{

class ImageView :
	public ImageViewBase,
	private InteractionHandler
{
	Q_OBJECT
public:
	/**
	 * \p content_rect is in virtual image coordinates.
	 * \p page_rect is in virtual image coordinates (may be null).
	 */
	ImageView(
		QImage const& image, QImage const& downscaled_image,
		ImageTransformation const& xform,
		QRectF const& content_rect,
		QRectF const& page_rect = QRectF());

	virtual ~ImageView();
signals:
	void manualContentRectSet(QRectF const& content_rect);

	void manualPageRectSet(QRectF const& page_rect);
private slots:
	void createContentBox();

	void removeContentBox();

	void createPageBox();

	void removePageBox();
private:
	enum Edge { LEFT = 1, RIGHT = 2, TOP = 4, BOTTOM = 8 };

	virtual void onPaint(QPainter& painter, InteractionState const& interaction);

	void onMouseDoubleClickEvent(QMouseEvent* event, InteractionState& interaction);

	void onMousePressEvent(QMouseEvent* event, InteractionState& interaction);

	void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction);

	void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction);

	void onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction);

	QPointF contentCornerPosition(int edge_mask) const;

	void contentCornerMoveRequest(int edge_mask, QPointF const& pos);

	QLineF contentEdgePosition(int edge) const;

	void contentEdgeMoveRequest(int edge, QLineF const& line);

	void contentDragFinished();

	QPointF pageCornerPosition(int edge_mask) const;

	void pageCornerMoveRequest(int edge_mask, QPointF const& pos);

	QLineF pageEdgePosition(int edge) const;

	void pageEdgeMoveRequest(int edge, QLineF const& line);

	void pageDragFinished();

	void forceInsideImage(QRectF& widget_rect, int edge_mask) const;

	void forceInsidePageRect(QRectF& widget_rect, int edge_mask) const;

	DraggablePoint m_contentCorners[4];
	ObjectDragHandler m_contentCornerHandlers[4];

	DraggableLineSegment m_contentEdges[4];
	ObjectDragHandler m_contentEdgeHandlers[4];

	DraggablePoint m_pageCorners[4];
	ObjectDragHandler m_pageCornerHandlers[4];

	DraggableLineSegment m_pageEdges[4];
	ObjectDragHandler m_pageEdgeHandlers[4];

	DragHandler m_dragHandler;
	ZoomHandler m_zoomHandler;

	QMenu* m_pNoContentMenu;
	QMenu* m_pHaveContentMenu;

	/**
	 * Content box in virtual image coordinates.
	 */
	QRectF m_contentRect;

	/**
	 * Page box in virtual image coordinates (null if page detection is off).
	 */
	QRectF m_pageRect;

	QSizeF m_minBoxSize;

	enum BoxDragTarget { DRAG_NONE, DRAG_CONTENT, DRAG_PAGE };
	BoxDragTarget m_boxDragTarget;
	QPointF m_boxDragStartPos;
	QRectF m_boxDragStartRect;
	InteractionState::Captor m_boxDragCaptor;
};

} // namespace select_content

#endif
