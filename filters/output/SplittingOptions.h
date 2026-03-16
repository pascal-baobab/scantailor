/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>

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

#ifndef OUTPUT_SPLITTINGOPTIONS_H_
#define OUTPUT_SPLITTINGOPTIONS_H_

#include <QDomElement>

namespace output
{

enum SplittingMode { BLACK_AND_WHITE_FOREGROUND, COLOR_FOREGROUND };

class SplittingOptions
{
public:
	SplittingOptions();

	explicit SplittingOptions(QDomElement const& el);

	QDomElement toXml(QDomDocument& doc, QString const& name) const;

	bool isSplitOutput() const { return m_isSplitOutput; }

	void setSplitOutput(bool splitOutput) { m_isSplitOutput = splitOutput; }

	SplittingMode getSplittingMode() const { return m_splittingMode; }

	void setSplittingMode(SplittingMode mode) { m_splittingMode = mode; }

	bool isOriginalBackgroundEnabled() const { return m_isOriginalBackgroundEnabled; }

	void setOriginalBackgroundEnabled(bool enable) { m_isOriginalBackgroundEnabled = enable; }

	bool operator==(SplittingOptions const& other) const;

	bool operator!=(SplittingOptions const& other) const;

private:
	static SplittingMode parseSplittingMode(QString const& str);

	static QString formatSplittingMode(SplittingMode mode);

	bool m_isSplitOutput;
	SplittingMode m_splittingMode;
	bool m_isOriginalBackgroundEnabled;
};

} // namespace output

#endif // OUTPUT_SPLITTINGOPTIONS_H_
