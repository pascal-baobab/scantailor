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

#ifndef OUTPUT_BLACK_WHITE_OPTIONS_H_
#define OUTPUT_BLACK_WHITE_OPTIONS_H_

class QString;
class QDomDocument;
class QDomElement;

namespace output
{

enum BinarizationMethod { OTSU, SAUVOLA, WOLF };

class BlackWhiteOptions
{
public:
	class ColorSegmenterOptions
	{
	public:
		ColorSegmenterOptions();
		explicit ColorSegmenterOptions(QDomElement const& el);
		QDomElement toXml(QDomDocument& doc, QString const& name) const;

		bool isEnabled() const { return m_isEnabled; }
		void setEnabled(bool enabled) { m_isEnabled = enabled; }

		int getNoiseReduction() const { return m_noiseReduction; }
		void setNoiseReduction(int val) { m_noiseReduction = val; }

		int getRedThresholdAdjustment() const { return m_redThresholdAdjustment; }
		void setRedThresholdAdjustment(int val) { m_redThresholdAdjustment = val; }

		int getGreenThresholdAdjustment() const { return m_greenThresholdAdjustment; }
		void setGreenThresholdAdjustment(int val) { m_greenThresholdAdjustment = val; }

		int getBlueThresholdAdjustment() const { return m_blueThresholdAdjustment; }
		void setBlueThresholdAdjustment(int val) { m_blueThresholdAdjustment = val; }

		bool operator==(ColorSegmenterOptions const& other) const;
		bool operator!=(ColorSegmenterOptions const& other) const;
	private:
		bool m_isEnabled;
		int m_noiseReduction;
		int m_redThresholdAdjustment;
		int m_greenThresholdAdjustment;
		int m_blueThresholdAdjustment;
	};

	BlackWhiteOptions();

	BlackWhiteOptions(QDomElement const& el);

	QDomElement toXml(QDomDocument& doc, QString const& name) const;

	bool whiteMargins() const { return m_whiteMargins; }

	void setWhiteMargins(bool val) { m_whiteMargins = val; }

	bool normalizeIllumination() const { return m_normalizeIllumination; }

	void setNormalizeIllumination(bool val) { m_normalizeIllumination = val; }

	int thresholdAdjustment() const { return m_thresholdAdjustment; }

	void setThresholdAdjustment(int val) { m_thresholdAdjustment = val; }

	BinarizationMethod binarizationMethod() const { return m_binarizationMethod; }

	void setBinarizationMethod(BinarizationMethod method) { m_binarizationMethod = method; }

	ColorSegmenterOptions const& getColorSegmenterOptions() const { return m_colorSegmenterOptions; }

	void setColorSegmenterOptions(ColorSegmenterOptions const& opts) { m_colorSegmenterOptions = opts; }

	int windowSize() const { return m_windowSize; }

	void setWindowSize(int val) { m_windowSize = val; }

	double sauvolaCoef() const { return m_sauvolaCoef; }

	void setSauvolaCoef(double val) { m_sauvolaCoef = val; }

	int wolfLowerBound() const { return m_wolfLowerBound; }

	void setWolfLowerBound(int val) { m_wolfLowerBound = val; }

	int wolfUpperBound() const { return m_wolfUpperBound; }

	void setWolfUpperBound(int val) { m_wolfUpperBound = val; }

	bool operator==(BlackWhiteOptions const& other) const;

	bool operator!=(BlackWhiteOptions const& other) const;
private:
	static BinarizationMethod parseBinarizationMethod(QString const& str);
	static QString formatBinarizationMethod(BinarizationMethod method);

	bool m_whiteMargins;
	bool m_normalizeIllumination;
	int m_thresholdAdjustment;
	BinarizationMethod m_binarizationMethod;
	ColorSegmenterOptions m_colorSegmenterOptions;
	int m_windowSize;
	double m_sauvolaCoef;
	int m_wolfLowerBound;
	int m_wolfUpperBound;
};

} // namespace output

#endif
