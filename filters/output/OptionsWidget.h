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

#ifndef OUTPUT_OPTIONSWIDGET_H_
#define OUTPUT_OPTIONSWIDGET_H_

#include "ui_OutputOptionsWidget.h"
#include "FilterOptionsWidget.h"
#include "IntrusivePtr.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "ColorParams.h"
#include "BlackWhiteOptions.h"
#include "ColorGrayscaleOptions.h"
#include "SplittingOptions.h"
#include "DewarpingMode.h"
#include "DepthPerception.h"
#include "DespeckleLevel.h"
#include "Dpi.h"
#include "ImageViewTab.h"
#include <set>

class QSpinBox;
class QLabel;

namespace dewarping
{
	class DistortionModel;
}

namespace output
{

class Settings;
class DewarpingParams;

class OptionsWidget
	: public FilterOptionsWidget, private Ui::OutputOptionsWidget
{
	Q_OBJECT
public:
	OptionsWidget(IntrusivePtr<Settings> const& settings,
		PageSelectionAccessor const& page_selection_accessor);
	
	~OptionsWidget() override;
	
	void preUpdateUI(PageId const& page_id);
	
	void postUpdateUI();

	ImageViewTab lastTab() const { return m_lastTab; }

	DepthPerception const& depthPerception() const { return m_depthPerception; }
signals:
	void despeckleLevelChanged(DespeckleLevel level, bool* handled);

	void depthPerceptionChanged(double val);

	void generatePdfChanged(bool enabled);

	void ocrLanguageChanged(QString const& languages);

	void pdfDpiChanged(int dpi);
public slots:
	void tabChanged(ImageViewTab tab);

	void distortionModelChanged(dewarping::DistortionModel const& model);
private slots:
	void changeDpiButtonClicked();
	
	void applyColorsButtonClicked();
	
	void dpiChanged(std::set<PageId> const& pages, Dpi const& dpi);

	void applyColorsConfirmed(std::set<PageId> const& pages);

	void colorModeChanged(int idx);
	
	void whiteMarginsToggled(bool checked);
	
	void equalizeIlluminationToggled(bool checked);
	
	void setLighterThreshold();
	
	void setDarkerThreshold();
	
	void setNeutralThreshold();
	
	void bwThresholdChanged();

	void thresholdMethodChanged(int idx);

	void windowSizeChanged(int val);

	void fillColorChanged(int idx);

	void bwWhiteMarginsToggled(bool checked);

	void bwEqualizeIlluminationToggled(bool checked);

	void despeckleOffSelected();

	void despeckleCautiousSelected();

	void despeckleNormalSelected();

	void despeckleAggressiveSelected();

	void applyDespeckleButtonClicked();

	void applyDespeckleConfirmed(std::set<PageId> const& pages);

	void changeDewarpingButtonClicked();

	void dewarpingChanged(std::set<PageId> const& pages, DewarpingMode const& mode);

	void applyDepthPerceptionButtonClicked();

	void applyDepthPerceptionConfirmed(std::set<PageId> const& pages);

	void depthPerceptionChangedSlot(int val);

	void splitOutputToggled(bool checked);

	void bwForegroundToggled(bool checked);

	void colorForegroundToggled(bool checked);

	void originalBackgroundToggled(bool checked);

	void colorSegmentationToggled(bool checked);

	void noiseReductionChanged(int val);

	void redAdjChanged(int val);

	void greenAdjChanged(int val);

	void blueAdjChanged(int val);

	void posterizationToggled(bool checked);

	void posterizeLevelChanged(int val);

	void posterizeNormalizeToggled(bool checked);

	void posterizeForceBWToggled(bool checked);

	void generatePdfToggled(bool checked);

	void ocrLanguageToggled();

	void pdfDpiSpinChanged(int val);

	void updatePdfSizeEstimate();
private:
	void handleDespeckleLevelChange(DespeckleLevel level);

	void reloadIfNecessary();

	void updateDpiDisplay();

	void updateColorsDisplay();

	void updateDewarpingDisplay();
	
	IntrusivePtr<Settings> m_ptrSettings;
	PageSelectionAccessor m_pageSelectionAccessor;
	PageId m_pageId;
	Dpi m_outputDpi;
	ColorParams m_colorParams;
	SplittingOptions m_splittingOptions;
	DepthPerception m_depthPerception;
	DewarpingMode m_dewarpingMode;
	DespeckleLevel m_despeckleLevel;
	ImageViewTab m_lastTab;
	int m_ignoreThresholdChanges;
	QSpinBox* m_windowSizeSB;
	QLabel* m_windowSizeLabel;
};

} // namespace output

#endif
