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

#include "LightColorScheme.h"
#include <QFile>
#include <QStyle>
#include <QStyleFactory>
#include <QRegularExpression>

static QString qssConvertPxToEm(QString const& stylesheet, double base, int precision)
{
	QString result;
	QRegularExpression pxToEm(R"((\d+(\.\d+)?)px)");

	QRegularExpressionMatchIterator iter = pxToEm.globalMatch(stylesheet);
	int prevIndex = 0;
	while (iter.hasNext()) {
		QRegularExpressionMatch match = iter.next();
		result.append(stylesheet.mid(prevIndex, match.capturedStart() - prevIndex));

		double value = match.captured(1).toDouble();
		value /= base;
		result.append(QString::number(value, 'f', precision)).append("em");

		prevIndex = match.capturedEnd();
	}
	result.append(stylesheet.mid(prevIndex));
	return result;
}

LightColorScheme::LightColorScheme()
{
	loadPalette();
	loadStyleSheet();
	loadColorParams();
}

QPalette const*
LightColorScheme::getPalette() const
{
	return &m_palette;
}

QString const*
LightColorScheme::getStyleSheet() const
{
	return &m_styleSheet;
}

ColorScheme::ColorParams const*
LightColorScheme::getColorParams() const
{
	return &m_customColors;
}

QStyle*
LightColorScheme::getStyle() const
{
	return QStyleFactory::create("Fusion");
}

void
LightColorScheme::loadPalette()
{
	m_palette.setColor(QPalette::Window, QColor("#F0F0F0"));
	m_palette.setColor(QPalette::WindowText, QColor("#303030"));
	m_palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#909090"));
	m_palette.setColor(QPalette::Base, QColor("#FCFCFC"));
	m_palette.setColor(QPalette::Disabled, QPalette::Base, QColor("#FAFAFA"));
	m_palette.setColor(QPalette::AlternateBase, m_palette.color(QPalette::Window));
	m_palette.setColor(QPalette::Disabled, QPalette::AlternateBase,
	                   m_palette.color(QPalette::Disabled, QPalette::Window));
	m_palette.setColor(QPalette::ToolTipBase, QColor("#FFFFCD"));
	m_palette.setColor(QPalette::ToolTipText, Qt::black);
	m_palette.setColor(QPalette::Text, m_palette.color(QPalette::WindowText));
	m_palette.setColor(QPalette::Disabled, QPalette::Text,
	                   m_palette.color(QPalette::Disabled, QPalette::WindowText));
	m_palette.setColor(QPalette::Light, Qt::white);
	m_palette.setColor(QPalette::Midlight, QColor("#F0F0F0"));
	m_palette.setColor(QPalette::Dark, QColor("#DADADA"));
	m_palette.setColor(QPalette::Mid, QColor("#CCCCCC"));
	m_palette.setColor(QPalette::Shadow, QColor("#BEBEBE"));
	m_palette.setColor(QPalette::Button, m_palette.color(QPalette::Base));
	m_palette.setColor(QPalette::Disabled, QPalette::Button,
	                   m_palette.color(QPalette::Disabled, QPalette::Base));
	m_palette.setColor(QPalette::ButtonText, m_palette.color(QPalette::WindowText));
	m_palette.setColor(QPalette::Disabled, QPalette::ButtonText,
	                   m_palette.color(QPalette::Disabled, QPalette::WindowText));
	m_palette.setColor(QPalette::BrightText, QColor("#F40000"));
	m_palette.setColor(QPalette::Link, QColor("#0000FF"));
	m_palette.setColor(QPalette::Highlight, QColor("#B5B5B5"));
	m_palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor("#DFDFDF"));
	m_palette.setColor(QPalette::HighlightedText, m_palette.color(QPalette::WindowText));
	m_palette.setColor(QPalette::Disabled, QPalette::HighlightedText,
	                   m_palette.color(QPalette::Disabled, QPalette::WindowText));
}

void
LightColorScheme::loadStyleSheet()
{
	QFile styleSheetFile(QString::fromUtf8(":/light_scheme/stylesheet/stylesheet.qss"));
	if (styleSheetFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		m_styleSheet = styleSheetFile.readAll();
		styleSheetFile.close();
	}

#ifdef _WIN32
	m_styleSheet = qssConvertPxToEm(m_styleSheet, 13, 4);
#else
	m_styleSheet = qssConvertPxToEm(m_styleSheet, 16, 4);
#endif
}

void
LightColorScheme::loadColorParams()
{
	m_customColors["ThumbnailSequenceSelectedItemBackground"] = QColor("#727272");
	m_customColors["ThumbnailSequenceSelectedItemText"] = Qt::white;
	m_customColors["ThumbnailSequenceItemText"] = Qt::black;
	m_customColors["ThumbnailSequenceSelectionLeaderBackground"] = QColor("#5E5E5E");
	m_customColors["OpenNewProjectBorder"] = QColor("#CCCCCC");
	m_customColors["ProcessingIndicationFade"] = QColor("#939393");
	m_customColors["ProcessingIndicationHead"] = m_palette.color(QPalette::WindowText);
	m_customColors["ProcessingIndicationTail"] = m_palette.color(QPalette::Highlight);
	m_customColors["StageListHead"] = m_customColors["ProcessingIndicationHead"];
	m_customColors["StageListTail"] = m_customColors["ProcessingIndicationTail"];
	m_customColors["FixDpiDialogErrorText"] = QColor("#FB0000");
}
