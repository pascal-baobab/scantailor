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

#ifndef OUTPUT_COLOR_GRAYSCALE_OPTIONS_H_
#define OUTPUT_COLOR_GRAYSCALE_OPTIONS_H_

class QString;
class QDomDocument;
class QDomElement;

namespace output
{

class ColorGrayscaleOptions
{
public:
	class PosterizationOptions
	{
	public:
		PosterizationOptions();
		explicit PosterizationOptions(QDomElement const& el);
		QDomElement toXml(QDomDocument& doc, QString const& name) const;

		bool isEnabled() const { return m_isEnabled; }
		void setEnabled(bool enabled) { m_isEnabled = enabled; }

		int getLevel() const { return m_level; }
		void setLevel(int level) { m_level = level; }

		bool isNormalizationEnabled() const { return m_normalizationEnabled; }
		void setNormalizationEnabled(bool val) { m_normalizationEnabled = val; }

		bool isForceBlackAndWhite() const { return m_forceBlackAndWhite; }
		void setForceBlackAndWhite(bool val) { m_forceBlackAndWhite = val; }

		bool operator==(PosterizationOptions const& other) const;
		bool operator!=(PosterizationOptions const& other) const;
	private:
		bool m_isEnabled;
		int m_level;
		bool m_normalizationEnabled;
		bool m_forceBlackAndWhite;
	};

	ColorGrayscaleOptions()
	: m_whiteMargins(false), m_normalizeIllumination(false) {}

	ColorGrayscaleOptions(QDomElement const& el);

	QDomElement toXml(QDomDocument& doc, QString const& name) const;

	bool whiteMargins() const { return m_whiteMargins; }

	void setWhiteMargins(bool val) { m_whiteMargins = val; }

	bool normalizeIllumination() const { return m_normalizeIllumination; }

	void setNormalizeIllumination(bool val) { m_normalizeIllumination = val; }

	PosterizationOptions const& getPosterizationOptions() const { return m_posterizationOptions; }

	void setPosterizationOptions(PosterizationOptions const& opts) { m_posterizationOptions = opts; }

	bool operator==(ColorGrayscaleOptions const& other) const;

	bool operator!=(ColorGrayscaleOptions const& other) const;
private:
	bool m_whiteMargins;
	bool m_normalizeIllumination;
	PosterizationOptions m_posterizationOptions;
};

} // namespace output

#endif
