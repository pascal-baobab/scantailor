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

#include "ZoneCreationInteraction.h"
#include "ZoneInteractionContext.h"
#include "EditableZoneSet.h"
#include "ImageViewBase.h"
#include <QCursor>
#include <QTransform>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QLinearGradient>
#include <Qt>
#include <QLineF>
#include <QDebug>
#include <functional>

ZoneCreationInteraction::ZoneCreationInteraction(
	ZoneInteractionContext& context, InteractionState& interaction)
:	m_rContext(context),
	m_dragHandler(context.imageView(),
		[this](InteractionState const& state) { return isDragHandlerPermitted(state); }),
	m_dragWatcher(m_dragHandler),
	m_zoomHandler(context.imageView(), [](InteractionState const&) { return true; }),
	m_ptrSpline(new EditableSpline),
	m_initialZoneCreationMode(context.getZoneCreationMode()),
	m_leftMouseButtonPressed(m_initialZoneCreationMode == ZoneCreationMode::LASSO),
	m_mouseButtonModifiers(Qt::NoModifier)
{
	QPointF const screen_mouse_pos(
		m_rContext.imageView().mapFromGlobal(QCursor::pos()) + QPointF(0.5, 0.5)
	);
	QTransform const from_screen(m_rContext.imageView().widgetToImage());
	m_nextVertexImagePos = from_screen.map(screen_mouse_pos);
	m_nextVertexImagePos_mid1 = m_nextVertexImagePos;
	m_nextVertexImagePos_mid2 = m_nextVertexImagePos;

	makeLastFollower(m_dragHandler);
	m_dragHandler.makeFirstFollower(m_dragWatcher);

	makeLastFollower(m_zoomHandler);

	interaction.capture(m_interaction);
	m_ptrSpline->appendVertex(m_nextVertexImagePos);

	updateStatusTip();
}

void
ZoneCreationInteraction::onPaint(QPainter& painter, InteractionState const& interaction)
{
	painter.setWorldMatrixEnabled(false);
	painter.setRenderHint(QPainter::Antialiasing);

	QTransform const to_screen(m_rContext.imageView().imageToWidget());
	QTransform const from_screen(m_rContext.imageView().widgetToImage());

	m_visualizer.drawSplines(painter, to_screen, m_rContext.zones());

	QPen solid_line_pen(m_visualizer.solidColor());
	solid_line_pen.setCosmetic(true);
	solid_line_pen.setWidthF(1.5);

	QLinearGradient gradient; // From inactive to active point.
	gradient.setColorAt(0.0, m_visualizer.solidColor());
	gradient.setColorAt(1.0, m_visualizer.highlightDarkColor());

	QPen gradient_pen;
	gradient_pen.setCosmetic(true);
	gradient_pen.setWidthF(1.5);

	painter.setPen(solid_line_pen);
	painter.setBrush(Qt::NoBrush);

	ZoneCreationMode mode = currentZoneCreationMode();

	if (mode == ZoneCreationMode::RECTANGULAR) {
		if (m_nextVertexImagePos != m_ptrSpline->firstVertex()->point()) {
			m_visualizer.drawVertex(painter, to_screen.map(m_nextVertexImagePos_mid1),
				m_visualizer.highlightBrightColor());
			m_visualizer.drawVertex(painter, to_screen.map(m_nextVertexImagePos_mid2),
				m_visualizer.highlightBrightColor());

			QPointF const first(to_screen.map(m_ptrSpline->firstVertex()->point()));

			QColor startColor = m_visualizer.solidColor();
			QColor stopColor = m_visualizer.highlightDarkColor();
			QColor midColor(
				(startColor.red() + stopColor.red()) / 2,
				(startColor.green() + stopColor.green()) / 2,
				(startColor.blue() + stopColor.blue()) / 2
			);

			QLinearGradient gradMid1;
			gradMid1.setColorAt(0.0, startColor);
			gradMid1.setColorAt(1.0, midColor);

			QLinearGradient gradMid2;
			gradMid2.setColorAt(0.0, midColor);
			gradMid2.setColorAt(1.0, stopColor);

			// first → mid1
			QLineF line1(to_screen.map(QLineF(m_ptrSpline->firstVertex()->point(), m_nextVertexImagePos_mid1)));
			gradMid1.setStart(line1.p1()); gradMid1.setFinalStop(line1.p2());
			gradient_pen.setBrush(gradMid1); painter.setPen(gradient_pen);
			painter.drawLine(line1);

			// mid1 → next
			QLineF line2(to_screen.map(QLineF(m_nextVertexImagePos_mid1, m_nextVertexImagePos)));
			gradMid2.setStart(line2.p1()); gradMid2.setFinalStop(line2.p2());
			gradient_pen.setBrush(gradMid2); painter.setPen(gradient_pen);
			painter.drawLine(line2);

			// first → mid2
			QLineF line3(to_screen.map(QLineF(m_ptrSpline->firstVertex()->point(), m_nextVertexImagePos_mid2)));
			gradMid1.setStart(line3.p1()); gradMid1.setFinalStop(line3.p2());
			gradient_pen.setBrush(gradMid1); painter.setPen(gradient_pen);
			painter.drawLine(line3);

			// mid2 → next
			QLineF line4(to_screen.map(QLineF(m_nextVertexImagePos_mid2, m_nextVertexImagePos)));
			gradMid2.setStart(line4.p1()); gradMid2.setFinalStop(line4.p2());
			gradient_pen.setBrush(gradMid2); painter.setPen(gradient_pen);
			painter.drawLine(line4);

			painter.setPen(solid_line_pen);
		}
	} else {
		for (EditableSpline::SegmentIterator it(*m_ptrSpline); it.hasNext(); ) {
			SplineSegment const segment(it.next());
			QLineF const line(to_screen.map(segment.toLine()));

			if (segment.prev == m_ptrSpline->firstVertex() &&
					segment.prev->point() == m_nextVertexImagePos) {
				gradient.setStart(line.p2());
				gradient.setFinalStop(line.p1());
				gradient_pen.setBrush(gradient);
				painter.setPen(gradient_pen);
				painter.drawLine(line);
				painter.setPen(solid_line_pen);
			} else {
				painter.drawLine(line);
			}
		}

		QLineF const line(
			to_screen.map(QLineF(m_ptrSpline->lastVertex()->point(), m_nextVertexImagePos))
		);
		gradient.setStart(line.p1());
		gradient.setFinalStop(line.p2());
		gradient_pen.setBrush(gradient);
		painter.setPen(gradient_pen);
		painter.drawLine(line);
	}

	m_visualizer.drawVertex(
		painter, to_screen.map(m_nextVertexImagePos), m_visualizer.highlightBrightColor()
	);
}

void
ZoneCreationInteraction::onKeyPressEvent(QKeyEvent* event, InteractionState& interaction)
{
	if (event->key() == Qt::Key_Escape) {
		makePeerPreceeder(*m_rContext.createDefaultInteraction());
		m_rContext.imageView().update();
		delete this;
		event->accept();
	}
}

void
ZoneCreationInteraction::onMousePressEvent(QMouseEvent* event, InteractionState& interaction)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}
	m_leftMouseButtonPressed = true;
	m_mouseButtonModifiers = event->modifiers();
	event->accept();
}

void
ZoneCreationInteraction::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}
	m_leftMouseButtonPressed = false;

	ZoneCreationMode mode = currentZoneCreationMode();

	if (m_dragWatcher.haveSignificantDrag()) {
		return;
	}

	QTransform const to_screen(m_rContext.imageView().imageToWidget());
	QTransform const from_screen(m_rContext.imageView().widgetToImage());
	QPointF const screen_mouse_pos(event->pos() + QPointF(0.5, 0.5));
	QPointF const image_mouse_pos(from_screen.map(screen_mouse_pos));

	if (mode == ZoneCreationMode::RECTANGULAR) {
		if (m_nextVertexImagePos != m_ptrSpline->firstVertex()->point()) {
			QPointF const first_point = m_ptrSpline->firstVertex()->point();
			m_ptrSpline.reset(new EditableSpline);
			m_ptrSpline->appendVertex(first_point);
			m_ptrSpline->appendVertex(m_nextVertexImagePos_mid1);
			m_ptrSpline->appendVertex(m_nextVertexImagePos);
			m_ptrSpline->appendVertex(m_nextVertexImagePos_mid2);
			m_ptrSpline->setBridged(true);
			m_rContext.zones().addZone(m_ptrSpline);
			m_rContext.zones().commit();
		}
		makePeerPreceeder(*m_rContext.createDefaultInteraction());
		m_rContext.imageView().update();
		delete this;
	} else {
		if (m_ptrSpline->hasAtLeastSegments(2) &&
				m_nextVertexImagePos == m_ptrSpline->firstVertex()->point()) {
			// Finishing the spline.
			m_ptrSpline->setBridged(true);
			m_rContext.zones().addZone(m_ptrSpline);
			m_rContext.zones().commit();

			makePeerPreceeder(*m_rContext.createDefaultInteraction());
			m_rContext.imageView().update();
			delete this;
		} else if (m_nextVertexImagePos == m_ptrSpline->lastVertex()->point()) {
			// Removing the last vertex.
			m_ptrSpline->lastVertex()->remove();
			if (!m_ptrSpline->firstVertex()) {
				makePeerPreceeder(*m_rContext.createDefaultInteraction());
				m_rContext.imageView().update();
				delete this;
			}
		} else {
			Proximity const prox(screen_mouse_pos, m_ptrSpline->lastVertex()->point());
			if (prox > interaction.proximityThreshold()) {
				m_ptrSpline->appendVertex(image_mouse_pos);
				updateStatusTip();
			}
		}
	}

	event->accept();
}

void
ZoneCreationInteraction::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction)
{
	QPointF const screen_mouse_pos(event->pos() + QPointF(0.5, 0.5));
	QTransform const to_screen(m_rContext.imageView().imageToWidget());
	QTransform const from_screen(m_rContext.imageView().widgetToImage());

	m_nextVertexImagePos = from_screen.map(screen_mouse_pos);

	QPointF const first(to_screen.map(m_ptrSpline->firstVertex()->point()));
	QPointF const last(to_screen.map(m_ptrSpline->lastVertex()->point()));

	ZoneCreationMode mode = currentZoneCreationMode();

	if (Proximity(last, screen_mouse_pos) <= interaction.proximityThreshold()) {
		m_nextVertexImagePos = m_ptrSpline->lastVertex()->point();
	} else if (m_ptrSpline->hasAtLeastSegments(2) || mode == ZoneCreationMode::RECTANGULAR) {
		if (Proximity(first, screen_mouse_pos) <= interaction.proximityThreshold()) {
			m_nextVertexImagePos = m_ptrSpline->firstVertex()->point();
			updateStatusTip();
		}
	}

	if (mode == ZoneCreationMode::RECTANGULAR &&
			m_nextVertexImagePos != m_ptrSpline->firstVertex()->point()) {
		qreal dx = screen_mouse_pos.x() - first.x();
		qreal dy = screen_mouse_pos.y() - first.y();

		QPointF mid1_screen(first.x(), screen_mouse_pos.y());
		QPointF mid2_screen(screen_mouse_pos.x(), first.y());

		// Order mid1/mid2 so the rectangle winds consistently
		if (((dx > 0) && (dy > 0)) || ((dx < 0) && (dy < 0))) {
			m_nextVertexImagePos_mid1 = from_screen.map(mid1_screen);
			m_nextVertexImagePos_mid2 = from_screen.map(mid2_screen);
		} else {
			m_nextVertexImagePos_mid2 = from_screen.map(mid1_screen);
			m_nextVertexImagePos_mid1 = from_screen.map(mid2_screen);
		}
	}

	// Lasso mode: append vertex while dragging
	if (mode == ZoneCreationMode::LASSO && m_leftMouseButtonPressed &&
			m_mouseButtonModifiers == Qt::NoModifier) {
		Proximity min_dist = interaction.proximityThreshold();
		if (Proximity(last, screen_mouse_pos) > min_dist &&
				Proximity(first, screen_mouse_pos) > min_dist) {
			m_ptrSpline->appendVertex(m_nextVertexImagePos);
		} else if (m_ptrSpline->hasAtLeastSegments(2) &&
				m_nextVertexImagePos == m_ptrSpline->firstVertex()->point()) {
			m_ptrSpline->setBridged(true);
			m_rContext.zones().addZone(m_ptrSpline);
			m_rContext.zones().commit();
			makePeerPreceeder(*m_rContext.createDefaultInteraction());
			m_rContext.imageView().update();
			delete this;
			return;
		}
	}

	m_rContext.imageView().update();
}

void
ZoneCreationInteraction::updateStatusTip()
{
	QString tip;
	ZoneCreationMode mode = currentZoneCreationMode();

	if (mode == ZoneCreationMode::RECTANGULAR) {
		tip = tr("Click to finish this rectangular zone.  ESC to cancel.");
	} else {
		if (m_ptrSpline->hasAtLeastSegments(2)) {
			if (m_nextVertexImagePos == m_ptrSpline->firstVertex()->point()) {
				tip = tr("Click to finish this zone.  ESC to cancel.");
			} else {
				tip = tr("Connect first and last points to finish this zone.  ESC to cancel.");
			}
		} else {
			tip = tr("Use Z, X, C keys to switch zone creation mode.  ESC to cancel.");
		}
	}

	m_interaction.setInteractionStatusTip(tip);
}

bool
ZoneCreationInteraction::isDragHandlerPermitted(InteractionState const& interaction) const
{
	return !((currentZoneCreationMode() == ZoneCreationMode::LASSO)
		&& m_leftMouseButtonPressed
		&& (m_mouseButtonModifiers == Qt::NoModifier));
}

ZoneCreationMode
ZoneCreationInteraction::currentZoneCreationMode() const
{
	switch (m_initialZoneCreationMode) {
		case ZoneCreationMode::RECTANGULAR:
			return ZoneCreationMode::RECTANGULAR;
		case ZoneCreationMode::POLYGONAL:
		case ZoneCreationMode::LASSO:
			switch (m_rContext.getZoneCreationMode()) {
				case ZoneCreationMode::POLYGONAL:
					return ZoneCreationMode::POLYGONAL;
				case ZoneCreationMode::LASSO:
					return ZoneCreationMode::LASSO;
				default:
					break;
			}
	}
	return ZoneCreationMode::POLYGONAL;
}
