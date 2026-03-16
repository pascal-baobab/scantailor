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

#include "Params.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include <QDomDocument>
#include <QDomElement>

namespace select_content
{

Params::Params(
	QRectF const& content_rect, QSizeF const& content_size_mm,
	Dependencies const& deps, AutoManualMode const content_detect_mode,
	QRectF const& page_rect, AutoManualMode const page_detect_mode)
:	m_contentRect(content_rect),
	m_contentSizeMM(content_size_mm),
	m_pageRect(page_rect),
	m_deps(deps),
	m_mode(content_detect_mode),
	m_pageDetectMode(page_detect_mode)
{
}

Params::Params(Dependencies const& deps)
:	m_deps(deps),
	m_mode(MODE_AUTO),
	m_pageDetectMode(MODE_DISABLED)
{
}

static AutoManualMode stringToMode(QString const& str)
{
	if (str == "manual") return MODE_MANUAL;
	if (str == "disabled") return MODE_DISABLED;
	return MODE_AUTO;
}

static QString modeToString(AutoManualMode mode)
{
	switch (mode) {
		case MODE_MANUAL: return "manual";
		case MODE_DISABLED: return "disabled";
		default: return "auto";
	}
}

Params::Params(QDomElement const& filter_el)
:	m_contentRect(
		XmlUnmarshaller::rectF(
			filter_el.namedItem("content-rect").toElement()
		)
	),
	m_contentSizeMM(
		XmlUnmarshaller::sizeF(
			filter_el.namedItem("content-size-mm").toElement()
		)
	),
	m_pageRect(
		XmlUnmarshaller::rectF(
			filter_el.namedItem("page-rect").toElement()
		)
	),
	m_deps(filter_el.namedItem("dependencies").toElement()),
	m_mode(stringToMode(filter_el.attribute("mode"))),
	m_pageDetectMode(stringToMode(filter_el.attribute("pageDetectMode")))
{
}

Params::~Params()
{
}

QDomElement
Params::toXml(QDomDocument& doc, QString const& name) const
{
	XmlMarshaller marshaller(doc);

	QDomElement el(doc.createElement(name));
	el.setAttribute("mode", modeToString(m_mode));
	el.setAttribute("pageDetectMode", modeToString(m_pageDetectMode));
	el.appendChild(marshaller.rectF(m_contentRect, "content-rect"));
	el.appendChild(marshaller.sizeF(m_contentSizeMM, "content-size-mm"));
	if (!m_pageRect.isNull()) {
		el.appendChild(marshaller.rectF(m_pageRect, "page-rect"));
	}
	el.appendChild(m_deps.toXml(doc, "dependencies"));
	return el;
}

} // namespace content_rect
