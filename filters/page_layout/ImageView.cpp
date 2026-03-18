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
#include "OptionsWidget.h"
#include "Margins.h"
#include "Settings.h"
#include "ImageTransformation.h"
#include "ImagePresentation.h"
#include "Utils.h"
#include "Guide.h"
#include "imageproc/PolygonUtils.h"
#include "foundation/Proximity.h"
#include <QPointF>
#include <QLineF>
#include <QPolygonF>
#include <QRect>
#include <QSize>
#include <QString>
#include <QPainter>
#include <QPainterPath>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <Qt>
#include <functional>
#include <algorithm>
#include <cmath>
#include <math.h>
#include <assert.h>

using namespace imageproc;

namespace page_layout
{

ImageView::ImageView(
	IntrusivePtr<Settings> const& settings, PageId const& page_id,
	QImage const& image, QImage const& downscaled_image,
	ImageTransformation const& xform,
	QRectF const& adapted_content_rect, OptionsWidget const& opt_widget)
:	ImageViewBase(
		image, downscaled_image,
		ImagePresentation(xform.transform(), xform.resultingPreCropArea()),
		Margins(5, 5, 5, 5)
	),
	m_dragHandler(*this),
	m_zoomHandler(*this),
	m_ptrSettings(settings),
	m_pageId(page_id),
	m_physXform(xform.origDpi()),
	m_innerRect(adapted_content_rect),
	m_aggregateHardSizeMM(settings->getAggregateHardSizeMM()),
	m_committedAggregateHardSizeMM(m_aggregateHardSizeMM),
	m_alignment(opt_widget.alignment()),
	m_leftRightLinked(opt_widget.leftRightLinked()),
	m_topBottomLinked(opt_widget.topBottomLinked()),
	m_guidesFreeIndex(0),
	m_contextMenu(new QMenu(this)),
	m_addHorizontalGuideAction(0),
	m_addVerticalGuideAction(0),
	m_removeAllGuidesAction(0),
	m_removeGuideUnderMouseAction(0),
	m_lastContextMenuPos(),
	m_guideUnderMouse(-1)
{
	setMouseTracking(true);
	
	interactionState().setDefaultStatusTip(
		tr("Resize margins by dragging any of the solid lines.")
	);

	// Setup interaction stuff.
	static int const masks_by_edge[] = { TOP, RIGHT, BOTTOM, LEFT };
	static int const masks_by_corner[] = { TOP|LEFT, TOP|RIGHT, BOTTOM|RIGHT, BOTTOM|LEFT };
	for (int i = 0; i < 4; ++i) {
		int const corner_mask = masks_by_corner[i];
		int const edge_mask = masks_by_edge[i];

		// Proximity priority - inner rect higher than middle, corners higher than edges.
		m_innerCorners[i].setProximityPriorityCallback(
			[]() { return 4; }
		);
		m_innerEdges[i].setProximityPriorityCallback(
			[]() { return 3; }
		);
		m_middleCorners[i].setProximityPriorityCallback(
			[]() { return 2; }
		);
		m_middleEdges[i].setProximityPriorityCallback(
			[]() { return 1; }
		);

		// Proximity.
		m_innerCorners[i].setProximityCallback(
			[this, corner_mask](QPointF const& pos) { return cornerProximity(corner_mask, &m_innerRect, pos); }
		);
		m_middleCorners[i].setProximityCallback(
			[this, corner_mask](QPointF const& pos) { return cornerProximity(corner_mask, &m_middleRect, pos); }
		);
		m_innerEdges[i].setProximityCallback(
			[this, edge_mask](QPointF const& pos) { return edgeProximity(edge_mask, &m_innerRect, pos); }
		);
		m_middleEdges[i].setProximityCallback(
			[this, edge_mask](QPointF const& pos) { return edgeProximity(edge_mask, &m_middleRect, pos); }
		);

		// Drag initiation.
		m_innerCorners[i].setDragInitiatedCallback(
			[this](QPointF const& pos) { dragInitiated(pos); }
		);
		m_middleCorners[i].setDragInitiatedCallback(
			[this](QPointF const& pos) { dragInitiated(pos); }
		);
		m_innerEdges[i].setDragInitiatedCallback(
			[this](QPointF const& pos) { dragInitiated(pos); }
		);
		m_middleEdges[i].setDragInitiatedCallback(
			[this](QPointF const& pos) { dragInitiated(pos); }
		);

		// Drag continuation.
		m_innerCorners[i].setDragContinuationCallback(
			[this, corner_mask](QPointF const& pos) { innerRectDragContinuation(corner_mask, pos); }
		);
		m_middleCorners[i].setDragContinuationCallback(
			[this, corner_mask](QPointF const& pos) { middleRectDragContinuation(corner_mask, pos); }
		);
		m_innerEdges[i].setDragContinuationCallback(
			[this, edge_mask](QPointF const& pos) { innerRectDragContinuation(edge_mask, pos); }
		);
		m_middleEdges[i].setDragContinuationCallback(
			[this, edge_mask](QPointF const& pos) { middleRectDragContinuation(edge_mask, pos); }
		);

		// Drag finishing.
		m_innerCorners[i].setDragFinishedCallback(
			[this](QPointF const&) { dragFinished(); }
		);
		m_middleCorners[i].setDragFinishedCallback(
			[this](QPointF const&) { dragFinished(); }
		);
		m_innerEdges[i].setDragFinishedCallback(
			[this](QPointF const&) { dragFinished(); }
		);
		m_middleEdges[i].setDragFinishedCallback(
			[this](QPointF const&) { dragFinished(); }
		);

		m_innerCornerHandlers[i].setObject(&m_innerCorners[i]);
		m_middleCornerHandlers[i].setObject(&m_middleCorners[i]);
		m_innerEdgeHandlers[i].setObject(&m_innerEdges[i]);
		m_middleEdgeHandlers[i].setObject(&m_middleEdges[i]);

		Qt::CursorShape corner_cursor = (i & 1) ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor;
		m_innerCornerHandlers[i].setProximityCursor(corner_cursor);
		m_innerCornerHandlers[i].setInteractionCursor(corner_cursor);
		m_middleCornerHandlers[i].setProximityCursor(corner_cursor);
		m_middleCornerHandlers[i].setInteractionCursor(corner_cursor);

		Qt::CursorShape edge_cursor = (i & 1) ? Qt::SizeHorCursor : Qt::SizeVerCursor;
		m_innerEdgeHandlers[i].setProximityCursor(edge_cursor);
		m_innerEdgeHandlers[i].setInteractionCursor(edge_cursor);
		m_middleEdgeHandlers[i].setProximityCursor(edge_cursor);
		m_middleEdgeHandlers[i].setInteractionCursor(edge_cursor);

		makeLastFollower(m_innerCornerHandlers[i]);
		makeLastFollower(m_innerEdgeHandlers[i]);
		makeLastFollower(m_middleCornerHandlers[i]);
		makeLastFollower(m_middleEdgeHandlers[i]);
	}

	rootInteractionHandler().makeLastFollower(*this);
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);

	setupContextMenuInteraction();
	setupGuides();

	recalcBoxesAndFit(opt_widget.marginsMM());
}

ImageView::~ImageView()
{
	for (std::map<int, DraggableLineSegment*>::iterator it = m_draggableGuides.begin();
	     it != m_draggableGuides.end(); ++it) {
		delete it->second;
	}
	for (std::map<int, ObjectDragHandler*>::iterator it = m_draggableGuideHandlers.begin();
	     it != m_draggableGuideHandlers.end(); ++it) {
		delete it->second;
	}
}

void
ImageView::marginsSetExternally(Margins const& margins_mm)
{
	AggregateSizeChanged const changed = commitHardMargins(margins_mm);
	
	recalcBoxesAndFit(margins_mm);
	
	invalidateThumbnails(changed);
}

void
ImageView::leftRightLinkToggled(bool const linked)
{
	m_leftRightLinked = linked;
	if (linked) {
		Margins margins_mm(calcHardMarginsMM());
		if (margins_mm.left() != margins_mm.right()) {
			double const new_margin = std::min(
				margins_mm.left(), margins_mm.right()
			);
			margins_mm.setLeft(new_margin);
			margins_mm.setRight(new_margin);
			
			AggregateSizeChanged const changed =
					commitHardMargins(margins_mm);
			
			recalcBoxesAndFit(margins_mm);
			emit marginsSetLocally(margins_mm);
			
			invalidateThumbnails(changed);
		}
	}
}

void
ImageView::topBottomLinkToggled(bool const linked)
{
	m_topBottomLinked = linked;
	if (linked) {
		Margins margins_mm(calcHardMarginsMM());
		if (margins_mm.top() != margins_mm.bottom()) {
			double const new_margin = std::min(
				margins_mm.top(), margins_mm.bottom()
			);
			margins_mm.setTop(new_margin);
			margins_mm.setBottom(new_margin);
			
			AggregateSizeChanged const changed =
					commitHardMargins(margins_mm);
			
			recalcBoxesAndFit(margins_mm);
			emit marginsSetLocally(margins_mm);
			
			invalidateThumbnails(changed);
		}
	}
}

void
ImageView::alignmentChanged(Alignment const& alignment)
{
	m_alignment = alignment;

	Settings::AggregateSizeChanged const size_changed =
		m_ptrSettings->setPageAlignment(m_pageId, alignment);

	recalcBoxesAndFit(calcHardMarginsMM());

	enableGuidesInteraction(!m_alignment.isNull());
	forceInscribeGuides();

	if (size_changed == Settings::AGGREGATE_SIZE_CHANGED) {
		emit invalidateAllThumbnails();
	} else {
		emit invalidateThumbnail(m_pageId);
	}
}

void
ImageView::aggregateHardSizeChanged()
{
	m_aggregateHardSizeMM = m_ptrSettings->getAggregateHardSizeMM();
	m_committedAggregateHardSizeMM = m_aggregateHardSizeMM;
	recalcOuterRect();
	updatePresentationTransform(FIT);
}

void
ImageView::onPaint(QPainter& painter, InteractionState const& interaction)
{	
	QColor bg_color;
	QColor fg_color;
	if (m_alignment.isNull()) {
		// "Align with other pages" is turned off.
		// Different color is useful on a thumbnail list to
		// distinguish "safe" pages from potentially problematic ones.
		bg_color = QColor(0x58, 0x7f, 0xf4, 70);
		fg_color = QColor(0x00, 0x52, 0xff);
	} else {
		bg_color = QColor(0xbb, 0x00, 0xff, 40);
		fg_color = QColor(0xbe, 0x5b, 0xec);
	}

	QPainterPath outer_outline;
	outer_outline.addPolygon(
		PolygonUtils::round(
			m_alignment.isNull() ? m_middleRect : m_outerRect
		)
	);
	
	QPainterPath content_outline;
	content_outline.addPolygon(PolygonUtils::round(m_innerRect));
	
	painter.setRenderHint(QPainter::Antialiasing, false);
	
	painter.setPen(Qt::NoPen);
	painter.setBrush(bg_color);
	painter.drawPath(outer_outline.subtracted(content_outline));
	
	QPen pen(fg_color);
	pen.setCosmetic(true);
	pen.setWidthF(2.0);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(m_middleRect);
	painter.drawRect(m_innerRect);
	
	if (!m_alignment.isNull()) {
		pen.setStyle(Qt::DashLine);
		painter.setPen(pen);
		painter.drawRect(m_outerRect);

		// Draw guides.
		if (!m_guides.empty()) {
			painter.setWorldTransform(QTransform());

			QPen guide_pen(QColor(0x00, 0x9d, 0x9f));
			guide_pen.setStyle(Qt::DashLine);
			guide_pen.setCosmetic(true);
			guide_pen.setWidthF(2.0);
			painter.setPen(guide_pen);
			painter.setBrush(Qt::NoBrush);

			for (std::map<int, Guide>::const_iterator it = m_guides.begin();
			     it != m_guides.end(); ++it) {
				QLineF const guide(widgetGuideLine(it->first));
				painter.drawLine(guide);
			}
		}
	}
}

Proximity
ImageView::cornerProximity(
	int const edge_mask, QRectF const* box, QPointF const& mouse_pos) const
{
	QRectF const r(virtualToWidget().mapRect(*box));
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

	return Proximity(pt, mouse_pos);
}

Proximity
ImageView::edgeProximity(
	int const edge_mask, QRectF const* box, QPointF const& mouse_pos) const
{
	QRectF const r(virtualToWidget().mapRect(*box));
	QLineF line;

	switch (edge_mask) {
		case TOP:
			line.setP1(r.topLeft());
			line.setP2(r.topRight());
			break;
		case BOTTOM:
			line.setP1(r.bottomLeft());
			line.setP2(r.bottomRight());
			break;
		case LEFT:
			line.setP1(r.topLeft());
			line.setP2(r.bottomLeft());
			break;
		case RIGHT:
			line.setP1(r.topRight());
			line.setP2(r.bottomRight());
			break;
		default:
			assert(!"Unreachable");
	}

	return Proximity::pointAndLineSegment(mouse_pos, line);
}

void
ImageView::dragInitiated(QPointF const& mouse_pos)
{
	m_beforeResizing.middleWidgetRect = virtualToWidget().mapRect(m_middleRect);
	m_beforeResizing.virtToWidget = virtualToWidget();
	m_beforeResizing.widgetToVirt = widgetToVirtual();
	m_beforeResizing.mousePos = mouse_pos;
	m_beforeResizing.focalPoint = getWidgetFocalPoint();
}

void
ImageView::innerRectDragContinuation(int edge_mask, QPointF const& mouse_pos)
{
	// What really happens when we resize the inner box is resizing
	// the middle box in the opposite direction and moving the scene
	// on screen so that the object being dragged is still under mouse.

	QPointF const delta(mouse_pos - m_beforeResizing.mousePos);
	qreal left_adjust = 0;
	qreal right_adjust = 0;
	qreal top_adjust = 0;
	qreal bottom_adjust = 0;

	if (edge_mask & LEFT) {
		left_adjust = delta.x();
		if (m_leftRightLinked) {
			right_adjust = -left_adjust;
		}
	} else if (edge_mask & RIGHT) {
		right_adjust = delta.x();
		if (m_leftRightLinked) {
			left_adjust = -right_adjust;
		}
	}
	if (edge_mask & TOP) {
		top_adjust = delta.y();
		if (m_topBottomLinked) {
			bottom_adjust = -top_adjust;
		}
	} else if (edge_mask & BOTTOM) {
		bottom_adjust = delta.y();
		if (m_topBottomLinked) {
			top_adjust = -bottom_adjust;
		}
	}

	QRectF widget_rect(m_beforeResizing.middleWidgetRect);
	widget_rect.adjust(-left_adjust, -top_adjust, -right_adjust, -bottom_adjust);

	m_middleRect = m_beforeResizing.widgetToVirt.mapRect(widget_rect);
	forceNonNegativeHardMargins(m_middleRect);
	widget_rect = m_beforeResizing.virtToWidget.mapRect(m_middleRect);

	qreal effective_dx = 0;
	qreal effective_dy = 0;

	QRectF const& old_widget_rect = m_beforeResizing.middleWidgetRect;
	if (edge_mask & LEFT) {
		effective_dx = old_widget_rect.left() - widget_rect.left();
	} else if (edge_mask & RIGHT) {
		effective_dx = old_widget_rect.right() - widget_rect.right();
	}
	if (edge_mask & TOP) {
		effective_dy = old_widget_rect.top() - widget_rect.top();
	} else if (edge_mask & BOTTOM) {
		effective_dy = old_widget_rect.bottom()- widget_rect.bottom();
	}

	// Updating the focal point is what makes the image move
	// as we drag an inner edge.
	QPointF fp(m_beforeResizing.focalPoint);
	fp += QPointF(effective_dx, effective_dy);
	setWidgetFocalPoint(fp);

	m_aggregateHardSizeMM = m_ptrSettings->getAggregateHardSizeMM(
		m_pageId, origRectToSizeMM(m_middleRect), m_alignment
	);

	recalcOuterRect();

	updatePresentationTransform(DONT_FIT);

	emit marginsSetLocally(calcHardMarginsMM());
}

void
ImageView::middleRectDragContinuation(int const edge_mask, QPointF const& mouse_pos)
{
	QPointF const delta(mouse_pos - m_beforeResizing.mousePos);
	qreal left_adjust = 0;
	qreal right_adjust = 0;
	qreal top_adjust = 0;
	qreal bottom_adjust = 0;

	QRectF const bounds(maxViewportRect());
	QRectF const old_middle_rect(m_beforeResizing.middleWidgetRect);

	if (edge_mask & LEFT) {
		left_adjust = delta.x();
		if (old_middle_rect.left() + left_adjust < bounds.left()) {
			left_adjust = bounds.left() - old_middle_rect.left();
		}
		if (m_leftRightLinked) {
			right_adjust = -left_adjust;
		}
	} else if (edge_mask & RIGHT) {
		right_adjust = delta.x();
		if (old_middle_rect.right() + right_adjust > bounds.right()) {
			right_adjust = bounds.right() - old_middle_rect.right();
		}
		if (m_leftRightLinked) {
			left_adjust = -right_adjust;
		}
	}
	if (edge_mask & TOP) {
		top_adjust = delta.y();
		if (old_middle_rect.top() + top_adjust < bounds.top()) {
			top_adjust = bounds.top() - old_middle_rect.top();
		}
		if (m_topBottomLinked) {
			bottom_adjust = -top_adjust;
		}
	} else if (edge_mask & BOTTOM) {
		bottom_adjust = delta.y();
		if (old_middle_rect.bottom() + bottom_adjust > bounds.bottom()) {
			bottom_adjust = bounds.bottom() - old_middle_rect.bottom();
		}
		if (m_topBottomLinked) {
			top_adjust = -bottom_adjust;
		}
	}

	{
		QRectF widget_rect(old_middle_rect);
		widget_rect.adjust(left_adjust, top_adjust, right_adjust, bottom_adjust);

		m_middleRect = m_beforeResizing.widgetToVirt.mapRect(widget_rect);
		forceNonNegativeHardMargins(m_middleRect); // invalidates widget_rect
	}

	m_aggregateHardSizeMM = m_ptrSettings->getAggregateHardSizeMM(
		m_pageId, origRectToSizeMM(m_middleRect), m_alignment
	);

	recalcOuterRect();

	updatePresentationTransform(DONT_FIT);

	emit marginsSetLocally(calcHardMarginsMM());
}

void
ImageView::dragFinished()
{
	AggregateSizeChanged const agg_size_changed(
		commitHardMargins(calcHardMarginsMM())
	);

	QRectF const extended_viewport(maxViewportRect().adjusted(-0.5, -0.5, 0.5, 0.5));
	if (extended_viewport.contains(m_beforeResizing.middleWidgetRect)) {
		updatePresentationTransform(FIT);
	} else {
		updatePresentationTransform(DONT_FIT);
	}

	invalidateThumbnails(agg_size_changed);
}

/**
 * Updates m_middleRect and m_outerRect based on \p margins_mm,
 * m_aggregateHardSizeMM and m_alignment, updates the displayed area.
 */
void
ImageView::recalcBoxesAndFit(Margins const& margins_mm)
{
	QTransform const virt_to_mm(virtualToImage() * m_physXform.pixelsToMM());
	QTransform const mm_to_virt(m_physXform.mmToPixels() * imageToVirtual());

	QPolygonF poly_mm(virt_to_mm.map(m_innerRect));
	Utils::extendPolyRectWithMargins(poly_mm, margins_mm);

	QRectF const middle_rect(mm_to_virt.map(poly_mm).boundingRect());
	
	QSizeF const hard_size_mm(
		QLineF(poly_mm[0], poly_mm[1]).length(),
		QLineF(poly_mm[0], poly_mm[3]).length()
	);
	Margins const soft_margins_mm(
		Utils::calcSoftMarginsMM(
			hard_size_mm, m_aggregateHardSizeMM, m_alignment
		)
	);
	
	Utils::extendPolyRectWithMargins(poly_mm, soft_margins_mm);

	QRectF const outer_rect(mm_to_virt.map(poly_mm).boundingRect());
	updateTransformAndFixFocalPoint(ImagePresentation(imageToVirtual(), outer_rect), CENTER_IF_FITS);

	m_middleRect = middle_rect;
	m_outerRect = outer_rect;

	forceInscribeGuides();
}

/**
 * Updates the virtual image area to be displayed by ImageViewBase,
 * optionally ensuring that this area completely fits into the view.
 *
 * \note virtualToImage() and imageToVirtual() are not affected by this.
 */
void
ImageView::updatePresentationTransform(FitMode const fit_mode)
{
	if (fit_mode == DONT_FIT) {
		updateTransformPreservingScale(ImagePresentation(imageToVirtual(), m_outerRect));
	} else {
		setZoomLevel(1.0);
		updateTransformAndFixFocalPoint(
			ImagePresentation(imageToVirtual(), m_outerRect), CENTER_IF_FITS
		);
	}
}

void
ImageView::forceNonNegativeHardMargins(QRectF& middle_rect) const
{
	if (middle_rect.left() > m_innerRect.left()) {
		middle_rect.setLeft(m_innerRect.left());
	}
	if (middle_rect.right() < m_innerRect.right()) {
		middle_rect.setRight(m_innerRect.right());
	}
	if (middle_rect.top() > m_innerRect.top()) {
		middle_rect.setTop(m_innerRect.top());
	}
	if (middle_rect.bottom() < m_innerRect.bottom()) {
		middle_rect.setBottom(m_innerRect.bottom());
	}
}

/**
 * \brief Calculates margins in millimeters between m_innerRect and m_middleRect.
 */
Margins
ImageView::calcHardMarginsMM() const
{
	QPointF const center(m_innerRect.center());
	
	QLineF const top_margin_line(
		QPointF(center.x(), m_middleRect.top()),
		QPointF(center.x(), m_innerRect.top())
	);
	
	QLineF const bottom_margin_line(
		QPointF(center.x(), m_innerRect.bottom()),
		QPointF(center.x(), m_middleRect.bottom())
	);
	
	QLineF const left_margin_line(
		QPointF(m_middleRect.left(), center.y()),
		QPointF(m_innerRect.left(), center.y())
	);
	
	QLineF const right_margin_line(
		QPointF(m_innerRect.right(), center.y()),
		QPointF(m_middleRect.right(), center.y())
	);
	
	QTransform const virt_to_mm(virtualToImage() * m_physXform.pixelsToMM());

	Margins margins;
	margins.setTop(virt_to_mm.map(top_margin_line).length());
	margins.setBottom(virt_to_mm.map(bottom_margin_line).length());
	margins.setLeft(virt_to_mm.map(left_margin_line).length());
	margins.setRight(virt_to_mm.map(right_margin_line).length());
	
	return margins;
}

/**
 * \brief Recalculates m_outerRect based on m_middleRect, m_aggregateHardSizeMM
 *        and m_alignment.
 */
void
ImageView::recalcOuterRect()
{
	QTransform const virt_to_mm(virtualToImage() * m_physXform.pixelsToMM());
	QTransform const mm_to_virt(m_physXform.mmToPixels() * imageToVirtual());

	QPolygonF poly_mm(virt_to_mm.map(m_middleRect));
	
	QSizeF const hard_size_mm(
		QLineF(poly_mm[0], poly_mm[1]).length(),
		QLineF(poly_mm[0], poly_mm[3]).length()
	);
	Margins const soft_margins_mm(
		Utils::calcSoftMarginsMM(
			hard_size_mm, m_aggregateHardSizeMM, m_alignment
		)
	);
	
	Utils::extendPolyRectWithMargins(poly_mm, soft_margins_mm);
	
	m_outerRect = mm_to_virt.map(poly_mm).boundingRect();

	forceInscribeGuides();
}

QSizeF
ImageView::origRectToSizeMM(QRectF const& rect) const
{
	QTransform const virt_to_mm(virtualToImage() * m_physXform.pixelsToMM());

	QLineF const hor_line(rect.topLeft(), rect.topRight());
	QLineF const vert_line(rect.topLeft(), rect.bottomLeft());
	
	QSizeF const size_mm(
		virt_to_mm.map(hor_line).length(),
		virt_to_mm.map(vert_line).length()
	);
	
	return size_mm;
}

ImageView::AggregateSizeChanged
ImageView::commitHardMargins(Margins const& margins_mm)
{
	m_ptrSettings->setHardMarginsMM(m_pageId, margins_mm);
	m_aggregateHardSizeMM = m_ptrSettings->getAggregateHardSizeMM();
	
	AggregateSizeChanged changed = AGGREGATE_SIZE_UNCHANGED;
	if (m_committedAggregateHardSizeMM != m_aggregateHardSizeMM) {
		 changed = AGGREGATE_SIZE_CHANGED;
	}
	
	m_committedAggregateHardSizeMM = m_aggregateHardSizeMM;
	
	return changed;
}

void
ImageView::invalidateThumbnails(AggregateSizeChanged const agg_size_changed)
{
	if (agg_size_changed == AGGREGATE_SIZE_CHANGED) {
		emit invalidateAllThumbnails();
	} else {
		emit invalidateThumbnail(m_pageId);
	}
}

void
ImageView::setupContextMenuInteraction()
{
	m_addHorizontalGuideAction = m_contextMenu->addAction(tr("Add a horizontal guide"));
	m_addVerticalGuideAction = m_contextMenu->addAction(tr("Add a vertical guide"));
	m_removeAllGuidesAction = m_contextMenu->addAction(tr("Remove all the guides"));
	m_removeGuideUnderMouseAction = m_contextMenu->addAction(tr("Remove this guide"));

	connect(m_addHorizontalGuideAction, &QAction::triggered, [this]() {
		addHorizontalGuide(widgetToGuideCs().map(m_lastContextMenuPos).y());
	});
	connect(m_addVerticalGuideAction, &QAction::triggered, [this]() {
		addVerticalGuide(widgetToGuideCs().map(m_lastContextMenuPos).x());
	});
	connect(m_removeAllGuidesAction, &QAction::triggered, [this]() {
		removeAllGuides();
	});
	connect(m_removeGuideUnderMouseAction, &QAction::triggered, [this]() {
		removeGuide(m_guideUnderMouse);
	});
}

void
ImageView::onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction)
{
	if (interaction.captured()) {
		return; // No context menus during resizing.
	}
	if (m_alignment.isNull()) {
		return;
	}

	QPointF const event_pos(QPointF(0.5, 0.5) + event->pos());
	if (!m_outerRect.contains(widgetToVirtual().map(event_pos))) {
		return;
	}

	m_guideUnderMouse = getGuideUnderMouse(event_pos, interaction);
	if (m_guideUnderMouse == -1) {
		m_addHorizontalGuideAction->setVisible(true);
		m_addVerticalGuideAction->setVisible(true);
		m_removeAllGuidesAction->setVisible(!m_guides.empty());
		m_removeGuideUnderMouseAction->setVisible(false);
		m_lastContextMenuPos = event_pos;
	} else {
		m_addHorizontalGuideAction->setVisible(false);
		m_addVerticalGuideAction->setVisible(false);
		m_removeAllGuidesAction->setVisible(false);
		m_removeGuideUnderMouseAction->setVisible(true);
	}

	m_contextMenu->popup(event->globalPos());
}

void
ImageView::setupGuides()
{
	QTransform const mm_to_virt(m_physXform.mmToPixels() * imageToVirtual());
	for (std::vector<Guide>::const_iterator it = m_ptrSettings->guides().begin();
	     it != m_ptrSettings->guides().end(); ++it) {
		m_guides[m_guidesFreeIndex] = Guide(mm_to_virt.map(static_cast<QLineF>(*it)));
		setupGuideInteraction(m_guidesFreeIndex++);
	}
}

void
ImageView::addHorizontalGuide(double y)
{
	if (m_alignment.isNull()) return;
	m_guides[m_guidesFreeIndex] = Guide(Qt::Horizontal, y);
	setupGuideInteraction(m_guidesFreeIndex++);
	update();
	syncGuidesSettings();
}

void
ImageView::addVerticalGuide(double x)
{
	if (m_alignment.isNull()) return;
	m_guides[m_guidesFreeIndex] = Guide(Qt::Vertical, x);
	setupGuideInteraction(m_guidesFreeIndex++);
	update();
	syncGuidesSettings();
}

void
ImageView::removeAllGuides()
{
	if (m_alignment.isNull()) return;

	for (std::map<int, ObjectDragHandler*>::iterator it = m_draggableGuideHandlers.begin();
	     it != m_draggableGuideHandlers.end(); ++it) {
		it->second->unlink();
		delete it->second;
	}
	for (std::map<int, DraggableLineSegment*>::iterator it = m_draggableGuides.begin();
	     it != m_draggableGuides.end(); ++it) {
		delete it->second;
	}
	m_draggableGuideHandlers.clear();
	m_draggableGuides.clear();
	m_guides.clear();
	m_guidesFreeIndex = 0;
	update();
	syncGuidesSettings();
}

void
ImageView::removeGuide(int index)
{
	if (m_alignment.isNull()) return;
	if (m_guides.find(index) == m_guides.end()) return;

	std::map<int, ObjectDragHandler*>::iterator hit = m_draggableGuideHandlers.find(index);
	if (hit != m_draggableGuideHandlers.end()) {
		hit->second->unlink();
		delete hit->second;
		m_draggableGuideHandlers.erase(hit);
	}
	std::map<int, DraggableLineSegment*>::iterator dit = m_draggableGuides.find(index);
	if (dit != m_draggableGuides.end()) {
		delete dit->second;
		m_draggableGuides.erase(dit);
	}
	m_guides.erase(index);
	update();
	syncGuidesSettings();
}

QTransform
ImageView::widgetToGuideCs() const
{
	QTransform xform(widgetToVirtual());
	xform *= QTransform().translate(-m_outerRect.center().x(), -m_outerRect.center().y());
	return xform;
}

QTransform
ImageView::guideToWidgetCs() const
{
	return widgetToGuideCs().inverted();
}

void
ImageView::syncGuidesSettings()
{
	QTransform const virt_to_mm(virtualToImage() * m_physXform.pixelsToMM());
	m_ptrSettings->guides().clear();
	for (std::map<int, Guide>::const_iterator it = m_guides.begin();
	     it != m_guides.end(); ++it) {
		m_ptrSettings->guides().push_back(
			Guide(virt_to_mm.map(static_cast<QLineF>(it->second)))
		);
	}
}

void
ImageView::setupGuideInteraction(int const index)
{
	DraggableLineSegment* draggable = new DraggableLineSegment();
	draggable->setProximityPriority(1);
	draggable->setPositionCallback(
		[this, index]() { return guidePosition(index); }
	);
	draggable->setMoveRequestCallback(
		[this, index](QLineF const& line) { guideMoveRequest(index, line); }
	);
	draggable->setDragFinishedCallback(
		[this](QPointF const& pos) { guideDragFinished(pos); }
	);
	m_draggableGuides[index] = draggable;

	ObjectDragHandler* handler = new ObjectDragHandler(draggable);
	Qt::CursorShape const cursor_shape =
		(m_guides[index].getOrientation() == Qt::Horizontal)
		? Qt::SplitVCursor : Qt::SplitHCursor;
	handler->setProximityCursor(cursor_shape);
	handler->setInteractionCursor(cursor_shape);
	handler->setProximityStatusTip(tr("Drag the guide."));
	m_draggableGuideHandlers[index] = handler;

	if (!m_alignment.isNull()) {
		makeLastFollower(*handler);
	}
}

QLineF
ImageView::guidePosition(int const index) const
{
	return widgetGuideLine(index);
}

void
ImageView::guideMoveRequest(int const index, QLineF const& line)
{
	QRectF const valid_area(virtualToWidget().mapRect(m_outerRect));

	QLineF moved = line;
	if (m_guides.at(index).getOrientation() == Qt::Horizontal) {
		double const line_pos = moved.y1();
		double const top = valid_area.top() - line_pos;
		double const bottom = line_pos - valid_area.bottom();
		if (top > 0.0) {
			moved.translate(0.0, top);
		} else if (bottom > 0.0) {
			moved.translate(0.0, -bottom);
		}
	} else {
		double const line_pos = moved.x1();
		double const left = valid_area.left() - line_pos;
		double const right = line_pos - valid_area.right();
		if (left > 0.0) {
			moved.translate(left, 0.0);
		} else if (right > 0.0) {
			moved.translate(-right, 0.0);
		}
	}

	m_guides[index] = Guide(widgetToGuideCs().map(moved));
	update();
}

void
ImageView::guideDragFinished(QPointF const&)
{
	syncGuidesSettings();
}

QLineF
ImageView::widgetGuideLine(int const index) const
{
	QRectF const widget_rect(viewport()->rect());
	Guide const& guide = m_guides.at(index);
	QLineF guide_line(guideToWidgetCs().map(static_cast<QLineF>(guide)));
	if (guide.getOrientation() == Qt::Horizontal) {
		guide_line = QLineF(widget_rect.left(), guide_line.y1(), widget_rect.right(), guide_line.y2());
	} else {
		guide_line = QLineF(guide_line.x1(), widget_rect.top(), guide_line.x2(), widget_rect.bottom());
	}
	return guide_line;
}

int
ImageView::getGuideUnderMouse(QPointF const& screen_mouse_pos, InteractionState const& state) const
{
	for (std::map<int, Guide>::const_iterator it = m_guides.begin();
	     it != m_guides.end(); ++it) {
		QLineF const guide(widgetGuideLine(it->first));
		if (Proximity::pointAndLineSegment(screen_mouse_pos, guide) <= state.proximityThreshold()) {
			return it->first;
		}
	}
	return -1;
}

void
ImageView::enableGuidesInteraction(bool const state)
{
	if (state) {
		for (std::map<int, ObjectDragHandler*>::iterator it = m_draggableGuideHandlers.begin();
		     it != m_draggableGuideHandlers.end(); ++it) {
			makeLastFollower(*(it->second));
		}
	} else {
		for (std::map<int, ObjectDragHandler*>::iterator it = m_draggableGuideHandlers.begin();
		     it != m_draggableGuideHandlers.end(); ++it) {
			it->second->unlink();
		}
	}
}

void
ImageView::forceInscribeGuides()
{
	if (m_alignment.isNull()) return;

	bool need_sync = false;
	for (std::map<int, Guide>::iterator it = m_guides.begin();
	     it != m_guides.end(); ++it) {
		Guide& guide = it->second;
		double const pos = guide.getPosition();
		if (guide.getOrientation() == Qt::Vertical) {
			if (std::abs(pos) > (m_outerRect.width() / 2)) {
				guide.setPosition(std::copysign(m_outerRect.width() / 2, pos));
				need_sync = true;
			}
		} else {
			if (std::abs(pos) > (m_outerRect.height() / 2)) {
				guide.setPosition(std::copysign(m_outerRect.height() / 2, pos));
				need_sync = true;
			}
		}
	}
	if (need_sync) {
		syncGuidesSettings();
	}
}

} // namespace page_layout
