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

#include "DefaultParams.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include <QDomDocument>
#include <QDomElement>

using namespace page_split;
using namespace output;

DefaultParams::DefaultParams()
{
}

DefaultParams::DefaultParams(
	FixOrientationParams const& fix_orientation,
	DeskewParams const& deskew,
	PageSplitParams const& page_split,
	SelectContentParams const& select_content,
	PageLayoutParams const& page_layout,
	OutputParams const& output)
:	m_fixOrientationParams(fix_orientation),
	m_deskewParams(deskew),
	m_pageSplitParams(page_split),
	m_selectContentParams(select_content),
	m_pageLayoutParams(page_layout),
	m_outputParams(output)
{
}

DefaultParams::DefaultParams(QDomElement const& el)
:	m_fixOrientationParams(el.namedItem("fix-orientation-params").toElement()),
	m_deskewParams(el.namedItem("deskew-params").toElement()),
	m_pageSplitParams(el.namedItem("page-split-params").toElement()),
	m_selectContentParams(el.namedItem("select-content-params").toElement()),
	m_pageLayoutParams(el.namedItem("page-layout-params").toElement()),
	m_outputParams(el.namedItem("output-params").toElement())
{
}

QDomElement
DefaultParams::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.appendChild(m_fixOrientationParams.toXml(doc, "fix-orientation-params"));
	el.appendChild(m_deskewParams.toXml(doc, "deskew-params"));
	el.appendChild(m_pageSplitParams.toXml(doc, "page-split-params"));
	el.appendChild(m_selectContentParams.toXml(doc, "select-content-params"));
	el.appendChild(m_pageLayoutParams.toXml(doc, "page-layout-params"));
	el.appendChild(m_outputParams.toXml(doc, "output-params"));
	return el;
}


// --- FixOrientationParams ---

DefaultParams::FixOrientationParams::FixOrientationParams()
{
}

DefaultParams::FixOrientationParams::FixOrientationParams(OrthogonalRotation const& image_rotation)
:	m_imageRotation(image_rotation)
{
}

DefaultParams::FixOrientationParams::FixOrientationParams(QDomElement const& el)
:	m_imageRotation(XmlUnmarshaller::rotation(el.namedItem("imageRotation").toElement()))
{
}

QDomElement
DefaultParams::FixOrientationParams::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.appendChild(XmlMarshaller(doc).rotation(m_imageRotation, "imageRotation"));
	return el;
}


// --- DeskewParams ---

DefaultParams::DeskewParams::DeskewParams()
:	m_deskewAngleDeg(0.0), m_mode(MODE_AUTO)
{
}

DefaultParams::DeskewParams::DeskewParams(double deskew_angle_deg, AutoManualMode mode)
:	m_deskewAngleDeg(deskew_angle_deg), m_mode(mode)
{
}

DefaultParams::DeskewParams::DeskewParams(QDomElement const& el)
:	m_deskewAngleDeg(el.attribute("deskewAngleDeg").toDouble()),
	m_mode((el.attribute("mode") == "manual") ? MODE_MANUAL : MODE_AUTO)
{
}

QDomElement
DefaultParams::DeskewParams::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.setAttribute("deskewAngleDeg", QString::number(m_deskewAngleDeg, 'g', 16));
	el.setAttribute("mode", (m_mode == MODE_AUTO) ? "auto" : "manual");
	return el;
}


// --- PageSplitParams ---

DefaultParams::PageSplitParams::PageSplitParams()
:	m_layoutType(AUTO_LAYOUT_TYPE)
{
}

DefaultParams::PageSplitParams::PageSplitParams(page_split::LayoutType layout_type)
:	m_layoutType(layout_type)
{
}

DefaultParams::PageSplitParams::PageSplitParams(QDomElement const& el)
:	m_layoutType(layoutTypeFromString(el.attribute("layoutType")))
{
}

QDomElement
DefaultParams::PageSplitParams::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.setAttribute("layoutType", layoutTypeToString(m_layoutType));
	return el;
}


// --- SelectContentParams ---

DefaultParams::SelectContentParams::SelectContentParams()
:	m_pageRectSize(QSizeF(210, 297)),
	m_contentDetectEnabled(true),
	m_fineTuneCorners(false)
{
}

DefaultParams::SelectContentParams::SelectContentParams(
	QSizeF const& page_rect_size, bool content_detect_enabled, bool fine_tune_corners)
:	m_pageRectSize(page_rect_size),
	m_contentDetectEnabled(content_detect_enabled),
	m_fineTuneCorners(fine_tune_corners)
{
}

DefaultParams::SelectContentParams::SelectContentParams(QDomElement const& el)
:	m_pageRectSize(XmlUnmarshaller::sizeF(el.namedItem("pageRectSize").toElement())),
	m_contentDetectEnabled(el.attribute("contentDetectEnabled") == "1"),
	m_fineTuneCorners(el.attribute("fineTuneCorners") == "1")
{
}

QDomElement
DefaultParams::SelectContentParams::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.appendChild(XmlMarshaller(doc).sizeF(m_pageRectSize, "pageRectSize"));
	el.setAttribute("contentDetectEnabled", m_contentDetectEnabled ? "1" : "0");
	el.setAttribute("fineTuneCorners", m_fineTuneCorners ? "1" : "0");
	return el;
}


// --- PageLayoutParams ---

DefaultParams::PageLayoutParams::PageLayoutParams()
:	m_hardMargins(10, 5, 10, 5)
{
}

DefaultParams::PageLayoutParams::PageLayoutParams(
	Margins const& hard_margins, page_layout::Alignment const& alignment)
:	m_hardMargins(hard_margins), m_alignment(alignment)
{
}

DefaultParams::PageLayoutParams::PageLayoutParams(QDomElement const& el)
:	m_hardMargins(XmlUnmarshaller::margins(el.namedItem("hardMargins").toElement())),
	m_alignment(el.namedItem("alignment").toElement())
{
}

QDomElement
DefaultParams::PageLayoutParams::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.appendChild(XmlMarshaller(doc).margins(m_hardMargins, "hardMargins"));
	el.appendChild(m_alignment.toXml(doc, "alignment"));
	return el;
}


// --- OutputParams ---

DefaultParams::OutputParams::OutputParams()
:	m_dpi(600, 600), m_despeckleLevel(1.0)
{
}

DefaultParams::OutputParams::OutputParams(
	Dpi const& dpi,
	output::ColorParams const& color_params,
	output::SplittingOptions const& splitting_options,
	output::DepthPerception const& depth_perception,
	double despeckle_level)
:	m_dpi(dpi),
	m_colorParams(color_params),
	m_splittingOptions(splitting_options),
	m_depthPerception(depth_perception),
	m_despeckleLevel(despeckle_level)
{
}

DefaultParams::OutputParams::OutputParams(QDomElement const& el)
:	m_dpi(XmlUnmarshaller::dpi(el.namedItem("dpi").toElement())),
	m_colorParams(el.namedItem("colorParams").toElement()),
	m_splittingOptions(el.namedItem("splittingOptions").toElement()),
	m_depthPerception(el.attribute("depthPerception").toDouble()),
	m_despeckleLevel(el.attribute("despeckleLevel").toDouble())
{
}

QDomElement
DefaultParams::OutputParams::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.appendChild(XmlMarshaller(doc).dpi(m_dpi, "dpi"));
	el.appendChild(m_colorParams.toXml(doc, "colorParams"));
	el.appendChild(m_splittingOptions.toXml(doc, "splittingOptions"));
	el.setAttribute("depthPerception", QString::number(m_depthPerception.value(), 'g', 16));
	el.setAttribute("despeckleLevel", QString::number(m_despeckleLevel, 'g', 16));
	return el;
}
