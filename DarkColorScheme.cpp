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

#include "DarkColorScheme.h"
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

DarkColorScheme::DarkColorScheme()
{
	loadPalette();
	loadStyleSheet();
	loadColorParams();
}

QPalette const*
DarkColorScheme::getPalette() const
{
	return &m_palette;
}

QString const*
DarkColorScheme::getStyleSheet() const
{
	return &m_styleSheet;
}

ColorScheme::ColorParams const*
DarkColorScheme::getColorParams() const
{
	return &m_customColors;
}

QStyle*
DarkColorScheme::getStyle() const
{
	return QStyleFactory::create("Fusion");
}

void
DarkColorScheme::loadPalette()
{
	m_palette.setColor(QPalette::Window, QColor("#535353"));
	m_palette.setColor(QPalette::WindowText, QColor("#DDDDDD"));
	m_palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#989898"));
	m_palette.setColor(QPalette::Base, QColor("#454545"));
	m_palette.setColor(QPalette::Disabled, QPalette::Base, QColor("#4D4D4D"));
	m_palette.setColor(QPalette::AlternateBase, m_palette.color(QPalette::Window));
	m_palette.setColor(QPalette::Disabled, QPalette::AlternateBase,
	                   m_palette.color(QPalette::Disabled, QPalette::Window));
	m_palette.setColor(QPalette::ToolTipBase, QColor("#707070"));
	m_palette.setColor(QPalette::ToolTipText, m_palette.color(QPalette::WindowText));
	m_palette.setColor(QPalette::Text, m_palette.color(QPalette::WindowText));
	m_palette.setColor(QPalette::Disabled, QPalette::Text,
	                   m_palette.color(QPalette::Disabled, QPalette::WindowText));
	m_palette.setColor(QPalette::Light, QColor("#666666"));
	m_palette.setColor(QPalette::Midlight, QColor("#535353"));
	m_palette.setColor(QPalette::Dark, QColor("#404040"));
	m_palette.setColor(QPalette::Mid, QColor("#333333"));
	m_palette.setColor(QPalette::Shadow, QColor("#262626"));
	m_palette.setColor(QPalette::Button, m_palette.color(QPalette::Base));
	m_palette.setColor(QPalette::Disabled, QPalette::Button,
	                   m_palette.color(QPalette::Disabled, QPalette::Base));
	m_palette.setColor(QPalette::ButtonText, m_palette.color(QPalette::WindowText));
	m_palette.setColor(QPalette::Disabled, QPalette::ButtonText,
	                   m_palette.color(QPalette::Disabled, QPalette::WindowText));
	m_palette.setColor(QPalette::BrightText, QColor("#FC5248"));
	m_palette.setColor(QPalette::Link, QColor("#4F95FC"));
	m_palette.setColor(QPalette::Highlight, QColor("#6B6B6B"));
	m_palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor("#5D5D5D"));
	m_palette.setColor(QPalette::HighlightedText, m_palette.color(QPalette::WindowText));
	m_palette.setColor(QPalette::Disabled, QPalette::HighlightedText,
	                   m_palette.color(QPalette::Disabled, QPalette::WindowText));
}

void
DarkColorScheme::loadStyleSheet()
{
	QFile styleSheetFile(QString::fromUtf8(":/dark_scheme/stylesheet/stylesheet.qss"));
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
DarkColorScheme::loadColorParams()
{
	m_customColors["ThumbnailSequenceSelectedItemBackground"] = QColor("#424242");
	m_customColors["ThumbnailSequenceSelectionLeaderBackground"] = QColor("#555555");
	m_customColors["OpenNewProjectBorder"] = QColor("#535353");
	m_customColors["ProcessingIndicationFade"] = QColor("#282828");
	m_customColors["ProcessingIndicationHead"] = m_palette.color(QPalette::WindowText);
	m_customColors["ProcessingIndicationTail"] = m_palette.color(QPalette::Highlight);
	m_customColors["StageListHead"] = m_customColors["ProcessingIndicationHead"];
	m_customColors["StageListTail"] = m_customColors["ProcessingIndicationTail"];
	m_customColors["FixDpiDialogErrorText"] = QColor("#F34941");
}
