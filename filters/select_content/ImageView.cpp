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

#include "ImageView.h"
#include "ImageTransformation.h"
#include "ImagePresentation.h"
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QString>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QCursor>
#include <QDebug>
#include <Qt>
#ifndef Q_MOC_RUN
#include <boost/bind.hpp>
#endif
#include <algorithm>

namespace select_content
{

ImageView::ImageView(
	QImage const& image, QImage const& downscaled_image,
	ImageTransformation const& xform, QRectF const& content_rect,
	QRectF const& page_rect)
:	ImageViewBase(
		image, downscaled_image,
		ImagePresentation(xform.transform(), xform.resultingPreCropArea())
	),
	m_dragHandler(*this),
	m_zoomHandler(*this),
	m_pNoContentMenu(new QMenu(this)),
	m_pHaveContentMenu(new QMenu(this)),
	m_contentRect(content_rect),
	m_pageRect(page_rect),
	m_minBoxSize(10.0, 10.0),
	m_boxDragTarget(DRAG_NONE)
{
	setMouseTracking(true);

	interactionState().setDefaultStatusTip(
		tr("Use the context menu to enable / disable the content box.")
	);

	QString const content_drag_tip(tr("Drag lines or corners to resize the content box."));
	QString const page_drag_tip(tr("Drag lines or corners to resize the page box."));

	// Setup content corner drag handlers.
	static int const masks_by_corner[] = { TOP|LEFT, TOP|RIGHT, BOTTOM|RIGHT, BOTTOM|LEFT };
	for (int i = 0; i < 4; ++i) {
		m_contentCorners[i].setPositionCallback(
			boost::bind(&ImageView::contentCornerPosition, this, masks_by_corner[i])
		);
		m_contentCorners[i].setMoveRequestCallback(
			boost::bind(&ImageView::contentCornerMoveRequest, this, masks_by_corner[i], _1)
		);
		m_contentCorners[i].setDragFinishedCallback(
			boost::bind(&ImageView::contentDragFinished, this)
		);
		m_contentCornerHandlers[i].setObject(&m_contentCorners[i]);
		m_contentCornerHandlers[i].setProximityStatusTip(content_drag_tip);
		Qt::CursorShape cursor = (i & 1) ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor;
		m_contentCornerHandlers[i].setProximityCursor(cursor);
		m_contentCornerHandlers[i].setInteractionCursor(cursor);
		makeLastFollower(m_contentCornerHandlers[i]);
	}

	// Setup content edge drag handlers.
	static int const masks_by_edge[] = { TOP, RIGHT, BOTTOM, LEFT };
	for (int i = 0; i < 4; ++i) {
		m_contentEdges[i].setPositionCallback(
			boost::bind(&ImageView::contentEdgePosition, this, masks_by_edge[i])
		);
		m_contentEdges[i].setMoveRequestCallback(
			boost::bind(&ImageView::contentEdgeMoveRequest, this, masks_by_edge[i], _1)
		);
		m_contentEdges[i].setDragFinishedCallback(
			boost::bind(&ImageView::contentDragFinished, this)
		);
		m_contentEdgeHandlers[i].setObject(&m_contentEdges[i]);
		m_contentEdgeHandlers[i].setProximityStatusTip(content_drag_tip);
		Qt::CursorShape cursor = (i & 1) ? Qt::SizeHorCursor : Qt::SizeVerCursor;
		m_contentEdgeHandlers[i].setProximityCursor(cursor);
		m_contentEdgeHandlers[i].setInteractionCursor(cursor);
		makeLastFollower(m_contentEdgeHandlers[i]);
	}

	// Setup page corner drag handlers.
	for (int i = 0; i < 4; ++i) {
		m_pageCorners[i].setPositionCallback(
			boost::bind(&ImageView::pageCornerPosition, this, masks_by_corner[i])
		);
		m_pageCorners[i].setMoveRequestCallback(
			boost::bind(&ImageView::pageCornerMoveRequest, this, masks_by_corner[i], _1)
		);
		m_pageCorners[i].setDragFinishedCallback(
			boost::bind(&ImageView::pageDragFinished, this)
		);
		m_pageCornerHandlers[i].setObject(&m_pageCorners[i]);
		m_pageCornerHandlers[i].setProximityStatusTip(page_drag_tip);
		Qt::CursorShape cursor = (i & 1) ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor;
		m_pageCornerHandlers[i].setProximityCursor(cursor);
		m_pageCornerHandlers[i].setInteractionCursor(cursor);
		makeLastFollower(m_pageCornerHandlers[i]);
	}

	// Setup page edge drag handlers.
	for (int i = 0; i < 4; ++i) {
		m_pageEdges[i].setPositionCallback(
			boost::bind(&ImageView::pageEdgePosition, this, masks_by_edge[i])
		);
		m_pageEdges[i].setMoveRequestCallback(
			boost::bind(&ImageView::pageEdgeMoveRequest, this, masks_by_edge[i], _1)
		);
		m_pageEdges[i].setDragFinishedCallback(
			boost::bind(&ImageView::pageDragFinished, this)
		);
		m_pageEdgeHandlers[i].setObject(&m_pageEdges[i]);
		m_pageEdgeHandlers[i].setProximityStatusTip(page_drag_tip);
		Qt::CursorShape cursor = (i & 1) ? Qt::SizeHorCursor : Qt::SizeVerCursor;
		m_pageEdgeHandlers[i].setProximityCursor(cursor);
		m_pageEdgeHandlers[i].setInteractionCursor(cursor);
		makeLastFollower(m_pageEdgeHandlers[i]);
	}

	rootInteractionHandler().makeLastFollower(*this);
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);

	QAction* create = m_pNoContentMenu->addAction(tr("Create Content Box"));
	QAction* remove = m_pHaveContentMenu->addAction(tr("Remove Content Box"));
	connect(create, SIGNAL(triggered(bool)), this, SLOT(createContentBox()));
	connect(remove, SIGNAL(triggered(bool)), this, SLOT(removeContentBox()));

	m_pNoContentMenu->addSeparator();
	m_pHaveContentMenu->addSeparator();

	QAction* create_page = m_pNoContentMenu->addAction(tr("Create Page Box"));
	QAction* create_page2 = m_pHaveContentMenu->addAction(tr("Create Page Box"));
	QAction* remove_page = m_pHaveContentMenu->addAction(tr("Remove Page Box"));
	connect(create_page, SIGNAL(triggered(bool)), this, SLOT(createPageBox()));
	connect(create_page2, SIGNAL(triggered(bool)), this, SLOT(createPageBox()));
	connect(remove_page, SIGNAL(triggered(bool)), this, SLOT(removePageBox()));
}

ImageView::~ImageView()
{
}

void
ImageView::createContentBox()
{
	if (!m_contentRect.isEmpty()) {
		return;
	}
	if (interactionState().captured()) {
		return;
	}

	QRectF const bounding = m_pageRect.isNull() ? virtualDisplayRect() : m_pageRect;
	QRectF content_rect(0, 0, bounding.width() * 0.7, bounding.height() * 0.7);
	content_rect.moveCenter(bounding.center());
	m_contentRect = content_rect;
	update();
	emit manualContentRectSet(m_contentRect);
}

void
ImageView::removeContentBox()
{
	if (m_contentRect.isEmpty()) {
		return;
	}
	if (interactionState().captured()) {
		return;
	}

	m_contentRect = QRectF();
	update();
	emit manualContentRectSet(m_contentRect);
}

void
ImageView::createPageBox()
{
	if (!m_pageRect.isNull()) {
		return;
	}
	if (interactionState().captured()) {
		return;
	}

	QRectF const virtual_rect(virtualDisplayRect());
	QRectF page_rect(0, 0, virtual_rect.width() * 0.9, virtual_rect.height() * 0.9);
	page_rect.moveCenter(virtual_rect.center());
	m_pageRect = page_rect;
	update();
	emit manualPageRectSet(m_pageRect);
}

void
ImageView::removePageBox()
{
	if (m_pageRect.isNull()) {
		return;
	}
	if (interactionState().captured()) {
		return;
	}

	m_pageRect = QRectF();
	update();
	emit manualPageRectSet(m_pageRect);
}

void
ImageView::onPaint(QPainter& painter, InteractionState const& interaction)
{
	painter.setRenderHints(QPainter::Antialiasing, true);

	// Draw the page bounding box (green).
	if (!m_pageRect.isNull()) {
		QPen page_pen(QColor(0x00, 0xaa, 0x00));
		page_pen.setWidth(1);
		page_pen.setCosmetic(true);
		painter.setPen(page_pen);
		painter.setBrush(QColor(0x00, 0xff, 0x00, 20));
		painter.drawRect(m_pageRect);
	}

	// Draw the content bounding box (blue).
	if (!m_contentRect.isNull()) {
		QPen pen(QColor(0x00, 0x00, 0xff));
		pen.setWidth(1);
		pen.setCosmetic(true);
		painter.setPen(pen);
		painter.setBrush(QColor(0x00, 0x00, 0xff, 50));
		painter.drawRect(m_contentRect);
	}
}

void
ImageView::onMouseDoubleClickEvent(QMouseEvent* event, InteractionState& interaction)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}
	if (interaction.captured()) {
		return;
	}

	QRectF const bounding = m_pageRect.isNull() ? virtualDisplayRect() : m_pageRect;
	if (bounding.isEmpty()) {
		return;
	}
	m_contentRect = bounding;
	update();
	emit manualContentRectSet(m_contentRect);
	event->accept();
}

void
ImageView::onMousePressEvent(QMouseEvent* event, InteractionState& interaction)
{
	if (event->button() == Qt::LeftButton
		&& (event->modifiers() & Qt::ShiftModifier)
		&& !interaction.captured())
	{
		QPointF const widget_pos(event->pos() + QPointF(0.5, 0.5));
		QRectF const content_widget(virtualToWidget().mapRect(m_contentRect));
		QRectF const page_widget(
			m_pageRect.isNull() ? QRectF() : virtualToWidget().mapRect(m_pageRect)
		);

		if (!m_contentRect.isNull() && content_widget.contains(widget_pos)) {
			m_boxDragTarget = DRAG_CONTENT;
			m_boxDragStartPos = widget_pos;
			m_boxDragStartRect = content_widget;
			interaction.capture(m_boxDragCaptor);
			event->accept();
			return;
		} else if (!m_pageRect.isNull() && page_widget.contains(widget_pos)) {
			m_boxDragTarget = DRAG_PAGE;
			m_boxDragStartPos = widget_pos;
			m_boxDragStartRect = page_widget;
			interaction.capture(m_boxDragCaptor);
			event->accept();
			return;
		}
	}
}

void
ImageView::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction)
{
	if (m_boxDragTarget == DRAG_NONE) {
		return;
	}

	QPointF const widget_pos(event->pos() + QPointF(0.5, 0.5));
	QPointF const delta = widget_pos - m_boxDragStartPos;
	QRectF moved = m_boxDragStartRect.translated(delta);

	QRectF const image_rect(getOccupiedWidgetRect());

	// Clamp to image bounds
	if (moved.left() < image_rect.left()) {
		moved.moveLeft(image_rect.left());
	}
	if (moved.right() > image_rect.right()) {
		moved.moveRight(image_rect.right());
	}
	if (moved.top() < image_rect.top()) {
		moved.moveTop(image_rect.top());
	}
	if (moved.bottom() > image_rect.bottom()) {
		moved.moveBottom(image_rect.bottom());
	}

	if (m_boxDragTarget == DRAG_CONTENT) {
		// Clamp content inside page rect
		if (!m_pageRect.isNull()) {
			QRectF const page_widget(virtualToWidget().mapRect(m_pageRect));
			if (moved.left() < page_widget.left()) {
				moved.moveLeft(page_widget.left());
			}
			if (moved.right() > page_widget.right()) {
				moved.moveRight(page_widget.right());
			}
			if (moved.top() < page_widget.top()) {
				moved.moveTop(page_widget.top());
			}
			if (moved.bottom() > page_widget.bottom()) {
				moved.moveBottom(page_widget.bottom());
			}
		}
		m_contentRect = widgetToVirtual().mapRect(moved);
	} else {
		m_pageRect = widgetToVirtual().mapRect(moved);
	}

	update();
	event->accept();
}

void
ImageView::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction)
{
	if (event->button() != Qt::LeftButton || m_boxDragTarget == DRAG_NONE) {
		return;
	}

	if (m_boxDragTarget == DRAG_CONTENT) {
		emit manualContentRectSet(m_contentRect);
	} else {
		emit manualPageRectSet(m_pageRect);
	}
	m_boxDragTarget = DRAG_NONE;
	m_boxDragCaptor.release();
	event->accept();
}

void
ImageView::onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction)
{
	if (interaction.captured()) {
		return;
	}

	if (m_contentRect.isEmpty()) {
		m_pNoContentMenu->popup(event->globalPos());
	} else {
		m_pHaveContentMenu->popup(event->globalPos());
	}
}

QPointF
ImageView::contentCornerPosition(int edge_mask) const
{
	QRectF const r(virtualToWidget().mapRect(m_contentRect));
	QPointF pt;

	if (edge_mask & TOP) {
		pt.setY(r.top());
	} else if (edge_mask & BOTTOM) {
		pt.setY(r.bottom());
	}

	if (edge_mask & LEFT) {
		pt.setX(r.left());
	} else if (edge_mask & RIGHT) {
		pt.setX(r.right());
	}

	return pt;
}

void
ImageView::contentCornerMoveRequest(int edge_mask, QPointF const& pos)
{
	QRectF r(virtualToWidget().mapRect(m_contentRect));
	qreal const minw = m_minBoxSize.width();
	qreal const minh = m_minBoxSize.height();

	if (edge_mask & TOP) {
		r.setTop(std::min(pos.y(), r.bottom() - minh));
	} else if (edge_mask & BOTTOM) {
		r.setBottom(std::max(pos.y(), r.top() + minh));
	}

	if (edge_mask & LEFT) {
		r.setLeft(std::min(pos.x(), r.right() - minw));
	} else if (edge_mask & RIGHT) {
		r.setRight(std::max(pos.x(), r.left() + minw));
	}

	forceInsideImage(r, edge_mask);
	if (!m_pageRect.isNull()) {
		forceInsidePageRect(r, edge_mask);
	}
	m_contentRect = widgetToVirtual().mapRect(r);
	update();
}

QLineF
ImageView::contentEdgePosition(int const edge) const
{
	QRectF const r(virtualToWidget().mapRect(m_contentRect));

	if (edge == TOP) {
		return QLineF(r.topLeft(), r.topRight());
	} else if (edge == BOTTOM) {
		return QLineF(r.bottomLeft(), r.bottomRight());
	} else if (edge == LEFT) {
		return QLineF(r.topLeft(), r.bottomLeft());
	} else {
		return QLineF(r.topRight(), r.bottomRight());
	}
}

void
ImageView::contentEdgeMoveRequest(int const edge, QLineF const& line)
{
	contentCornerMoveRequest(edge, line.p1());
}

void
ImageView::contentDragFinished()
{
	emit manualContentRectSet(m_contentRect);
}

QPointF
ImageView::pageCornerPosition(int edge_mask) const
{
	QRectF const r(virtualToWidget().mapRect(m_pageRect));
	QPointF pt;

	if (edge_mask & TOP) {
		pt.setY(r.top());
	} else if (edge_mask & BOTTOM) {
		pt.setY(r.bottom());
	}

	if (edge_mask & LEFT) {
		pt.setX(r.left());
	} else if (edge_mask & RIGHT) {
		pt.setX(r.right());
	}

	return pt;
}

void
ImageView::pageCornerMoveRequest(int edge_mask, QPointF const& pos)
{
	QRectF r(virtualToWidget().mapRect(m_pageRect));
	qreal const minw = m_minBoxSize.width();
	qreal const minh = m_minBoxSize.height();

	if (edge_mask & TOP) {
		r.setTop(std::min(pos.y(), r.bottom() - minh));
	} else if (edge_mask & BOTTOM) {
		r.setBottom(std::max(pos.y(), r.top() + minh));
	}

	if (edge_mask & LEFT) {
		r.setLeft(std::min(pos.x(), r.right() - minw));
	} else if (edge_mask & RIGHT) {
		r.setRight(std::max(pos.x(), r.left() + minw));
	}

	forceInsideImage(r, edge_mask);
	m_pageRect = widgetToVirtual().mapRect(r);
	update();
}

QLineF
ImageView::pageEdgePosition(int const edge) const
{
	QRectF const r(virtualToWidget().mapRect(m_pageRect));

	if (edge == TOP) {
		return QLineF(r.topLeft(), r.topRight());
	} else if (edge == BOTTOM) {
		return QLineF(r.bottomLeft(), r.bottomRight());
	} else if (edge == LEFT) {
		return QLineF(r.topLeft(), r.bottomLeft());
	} else {
		return QLineF(r.topRight(), r.bottomRight());
	}
}

void
ImageView::pageEdgeMoveRequest(int const edge, QLineF const& line)
{
	pageCornerMoveRequest(edge, line.p1());
}

void
ImageView::pageDragFinished()
{
	emit manualPageRectSet(m_pageRect);
}

void
ImageView::forceInsideImage(QRectF& widget_rect, int const edge_mask) const
{
	qreal const minw = m_minBoxSize.width();
	qreal const minh = m_minBoxSize.height();
	QRectF const image_rect(getOccupiedWidgetRect());

	if ((edge_mask & LEFT) && widget_rect.left() < image_rect.left()) {
		widget_rect.setLeft(image_rect.left());
		widget_rect.setRight(std::max(widget_rect.right(), widget_rect.left() + minw));
	}
	if ((edge_mask & RIGHT) && widget_rect.right() > image_rect.right()) {
		widget_rect.setRight(image_rect.right());
		widget_rect.setLeft(std::min(widget_rect.left(), widget_rect.right() - minw));
	}
	if ((edge_mask & TOP) && widget_rect.top() < image_rect.top()) {
		widget_rect.setTop(image_rect.top());
		widget_rect.setBottom(std::max(widget_rect.bottom(), widget_rect.top() + minh));
	}
	if ((edge_mask & BOTTOM) && widget_rect.bottom() > image_rect.bottom()) {
		widget_rect.setBottom(image_rect.bottom());
		widget_rect.setTop(std::min(widget_rect.top(), widget_rect.bottom() - minh));
	}
}

void
ImageView::forceInsidePageRect(QRectF& widget_rect, int const edge_mask) const
{
	qreal const minw = m_minBoxSize.width();
	qreal const minh = m_minBoxSize.height();
	QRectF const page_rect(virtualToWidget().mapRect(m_pageRect));

	if ((edge_mask & LEFT) && widget_rect.left() < page_rect.left()) {
		widget_rect.setLeft(page_rect.left());
		widget_rect.setRight(std::max(widget_rect.right(), widget_rect.left() + minw));
	}
	if ((edge_mask & RIGHT) && widget_rect.right() > page_rect.right()) {
		widget_rect.setRight(page_rect.right());
		widget_rect.setLeft(std::min(widget_rect.left(), widget_rect.right() - minw));
	}
	if ((edge_mask & TOP) && widget_rect.top() < page_rect.top()) {
		widget_rect.setTop(page_rect.top());
		widget_rect.setBottom(std::max(widget_rect.bottom(), widget_rect.top() + minh));
	}
	if ((edge_mask & BOTTOM) && widget_rect.bottom() > page_rect.bottom()) {
		widget_rect.setBottom(page_rect.bottom());
		widget_rect.setTop(std::min(widget_rect.top(), widget_rect.bottom() - minh));
	}
}

} // namespace select_content
