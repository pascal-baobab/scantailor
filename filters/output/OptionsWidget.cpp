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

#include "OptionsWidget.h"
#include "BlackWhiteOptions.h"
#include "ChangeDpiDialog.h"
#include "ChangeDewarpingDialog.h"
#include "ApplyColorsDialog.h"
#include "Settings.h"
#include "Params.h"
#include "dewarping/DistortionModel.h"
#include "DespeckleLevel.h"
#include "ZoneSet.h"
#include "PictureZoneComparator.h"
#include "FillZoneComparator.h"
#include "../../Utils.h"
#include "ScopedIncDec.h"
#include "config.h"
#include <QtGlobal>
#include <QVariant>
#include <QColorDialog>
#include <QToolTip>
#include <QString>
#include <QCursor>
#include <QPoint>
#include <QSize>
#include <Qt>
#include <QDebug>
#include <QSpinBox>
#include <QHBoxLayout>

namespace output
{

OptionsWidget::OptionsWidget(
	IntrusivePtr<Settings> const& settings,
	PageSelectionAccessor const& page_selection_accessor)
:	m_ptrSettings(settings),
	m_pageSelectionAccessor(page_selection_accessor),
	m_despeckleLevel(DESPECKLE_NORMAL),
	m_lastTab(TAB_OUTPUT),
	m_ignoreThresholdChanges(0)
{
	setupUi(this);

	depthPerceptionSlider->setMinimum(qRound(DepthPerception::minValue() * 10));
	depthPerceptionSlider->setMaximum(qRound(DepthPerception::maxValue() * 10));
	
	colorModeSelector->addItem(tr("Black and White"), ColorParams::BLACK_AND_WHITE);
	colorModeSelector->addItem(tr("Color / Grayscale"), ColorParams::COLOR_GRAYSCALE);
	colorModeSelector->addItem(tr("Mixed"), ColorParams::MIXED);

	thresholdMethodBox->addItem(tr("Otsu"), OTSU);
	thresholdMethodBox->addItem(tr("Sauvola"), SAUVOLA);
	thresholdMethodBox->addItem(tr("Wolf"), WOLF);

	// Window size spinbox for Sauvola/Wolf binarization
	m_windowSizeLabel = new QLabel(tr("Window size:"), this);
	m_windowSizeSB = new QSpinBox(this);
	m_windowSizeSB->setRange(3, 999);
	m_windowSizeSB->setSingleStep(2);
	m_windowSizeSB->setValue(51);
	QHBoxLayout* windowSizeLayout = new QHBoxLayout();
	windowSizeLayout->addWidget(m_windowSizeLabel);
	windowSizeLayout->addWidget(m_windowSizeSB);
	// Insert after the binarization method row
	QLayout* parentLayout = thresholdMethodBox->parentWidget()->layout();
	if (QBoxLayout* box = qobject_cast<QBoxLayout*>(parentLayout)) {
		// Find the binarization layout and insert after it
		int idx = -1;
		for (int i = 0; i < box->count(); ++i) {
			QLayoutItem* item = box->itemAt(i);
			if (item->layout()) {
				// Check if this layout contains thresholdMethodBox
				for (int j = 0; j < item->layout()->count(); ++j) {
					if (item->layout()->itemAt(j)->widget() == thresholdMethodBox) {
						idx = i;
						break;
					}
				}
			}
			if (idx >= 0) break;
		}
		if (idx >= 0) {
			box->insertLayout(idx + 1, windowSizeLayout);
		} else {
			box->addLayout(windowSizeLayout);
		}
	}
	m_windowSizeLabel->setVisible(false);
	m_windowSizeSB->setVisible(false);

	fillColorBox->addItem(tr("White"), FILL_WHITE);
	fillColorBox->addItem(tr("Background"), FILL_BACKGROUND);
	
	darkerThresholdLink->setText(
		Utils::richTextForLink(darkerThresholdLink->text())
	);
	lighterThresholdLink->setText(
		Utils::richTextForLink(lighterThresholdLink->text())
	);
	thresholdSlider->setToolTip(QString::number(thresholdSlider->value()));
	
	updateDpiDisplay();
	updateColorsDisplay();
	updateDewarpingDisplay();
	
	connect(
		changeDpiButton, SIGNAL(clicked()),
		this, SLOT(changeDpiButtonClicked())
	);
	connect(
		colorModeSelector, SIGNAL(currentIndexChanged(int)),
		this, SLOT(colorModeChanged(int))
	);
	connect(
		whiteMarginsCB, SIGNAL(clicked(bool)),
		this, SLOT(whiteMarginsToggled(bool))
	);
	connect(
		equalizeIlluminationCB, SIGNAL(clicked(bool)),
		this, SLOT(equalizeIlluminationToggled(bool))
	);
	connect(
		lighterThresholdLink, SIGNAL(linkActivated(QString const&)),
		this, SLOT(setLighterThreshold())
	);
	connect(
		darkerThresholdLink, SIGNAL(linkActivated(QString const&)),
		this, SLOT(setDarkerThreshold())
	);
	connect(
		neutralThresholdBtn, SIGNAL(clicked()),
		this, SLOT(setNeutralThreshold())
	);
	connect(
		fillColorBox, SIGNAL(currentIndexChanged(int)),
		this, SLOT(fillColorChanged(int))
	);
	connect(
		bwWhiteMarginsCB, SIGNAL(clicked(bool)),
		this, SLOT(bwWhiteMarginsToggled(bool))
	);
	connect(
		bwEqualizeIlluminationCB, SIGNAL(clicked(bool)),
		this, SLOT(bwEqualizeIlluminationToggled(bool))
	);
	connect(
		thresholdMethodBox, SIGNAL(currentIndexChanged(int)),
		this, SLOT(thresholdMethodChanged(int))
	);
	connect(
		m_windowSizeSB, SIGNAL(valueChanged(int)),
		this, SLOT(windowSizeChanged(int))
	);
	connect(
		thresholdSlider, SIGNAL(valueChanged(int)),
		this, SLOT(bwThresholdChanged())
	);
	connect(
		thresholdSlider, SIGNAL(sliderReleased()),
		this, SLOT(bwThresholdChanged())
	);
	connect(
		applyColorsButton, SIGNAL(clicked()),
		this, SLOT(applyColorsButtonClicked())
	);

	connect(
		changeDewarpingButton, SIGNAL(clicked()),
		this, SLOT(changeDewarpingButtonClicked())
	);

	connect(
		applyDepthPerceptionButton, SIGNAL(clicked()),
		this, SLOT(applyDepthPerceptionButtonClicked())
	);

	connect(
		despeckleOffBtn, SIGNAL(clicked()),
		this, SLOT(despeckleOffSelected())
	);
	connect(
		despeckleCautiousBtn, SIGNAL(clicked()),
		this, SLOT(despeckleCautiousSelected())
	);
	connect(
		despeckleNormalBtn, SIGNAL(clicked()),
		this, SLOT(despeckleNormalSelected())
	);
	connect(
		despeckleAggressiveBtn, SIGNAL(clicked()),
		this, SLOT(despeckleAggressiveSelected())
	);
	connect(
		applyDespeckleButton, SIGNAL(clicked()),
		this, SLOT(applyDespeckleButtonClicked())
	);

	connect(
		depthPerceptionSlider, SIGNAL(valueChanged(int)),
		this, SLOT(depthPerceptionChangedSlot(int))
	);

	connect(
		colorSegmentationPanel, SIGNAL(toggled(bool)),
		this, SLOT(colorSegmentationToggled(bool))
	);
	connect(
		noiseReductionSB, SIGNAL(valueChanged(int)),
		this, SLOT(noiseReductionChanged(int))
	);
	connect(
		redAdjSB, SIGNAL(valueChanged(int)),
		this, SLOT(redAdjChanged(int))
	);
	connect(
		greenAdjSB, SIGNAL(valueChanged(int)),
		this, SLOT(greenAdjChanged(int))
	);
	connect(
		blueAdjSB, SIGNAL(valueChanged(int)),
		this, SLOT(blueAdjChanged(int))
	);
	connect(
		posterizationPanel, SIGNAL(toggled(bool)),
		this, SLOT(posterizationToggled(bool))
	);
	connect(
		posterizeLevelSB, SIGNAL(valueChanged(int)),
		this, SLOT(posterizeLevelChanged(int))
	);
	connect(
		posterizeNormalizeCB, SIGNAL(toggled(bool)),
		this, SLOT(posterizeNormalizeToggled(bool))
	);
	connect(
		posterizeForceBWCB, SIGNAL(toggled(bool)),
		this, SLOT(posterizeForceBWToggled(bool))
	);
	connect(
		splittingPanel, SIGNAL(toggled(bool)),
		this, SLOT(splitOutputToggled(bool))
	);
	connect(
		bwForegroundRB, SIGNAL(toggled(bool)),
		this, SLOT(bwForegroundToggled(bool))
	);
	connect(
		colorForegroundRB, SIGNAL(toggled(bool)),
		this, SLOT(colorForegroundToggled(bool))
	);
	connect(
		originalBackgroundCB, SIGNAL(toggled(bool)),
		this, SLOT(originalBackgroundToggled(bool))
	);
	connect(
		generatePdfCB, SIGNAL(toggled(bool)),
		this, SLOT(generatePdfToggled(bool))
	);
	connect(
		ocrEngCB, SIGNAL(toggled(bool)),
		this, SLOT(ocrLanguageToggled())
	);
	connect(
		ocrItaCB, SIGNAL(toggled(bool)),
		this, SLOT(ocrLanguageToggled())
	);
	connect(
		ocrFraCB, SIGNAL(toggled(bool)),
		this, SLOT(ocrLanguageToggled())
	);
	connect(
		ocrDeuCB, SIGNAL(toggled(bool)),
		this, SLOT(ocrLanguageToggled())
	);
	connect(
		ocrSpaCB, SIGNAL(toggled(bool)),
		this, SLOT(ocrLanguageToggled())
	);
	connect(
		ocrRusCB, SIGNAL(toggled(bool)),
		this, SLOT(ocrLanguageToggled())
	);
	connect(
		pdfDpiSpin, SIGNAL(valueChanged(int)),
		this, SLOT(pdfDpiSpinChanged(int))
	);

	pdfOptionsWidget->setVisible(generatePdfCB->isChecked());
	updatePdfSizeEstimate();

	thresholdSlider->setMinimum(-50);
	thresholdSlider->setMaximum(50);
	thresholLabel->setText(QString::number(thresholdSlider->value()));
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(PageId const& page_id)
{
	Params const params(m_ptrSettings->getParams(page_id));
	m_pageId = page_id;
	m_outputDpi = params.outputDpi();
	m_colorParams = params.colorParams();
	m_splittingOptions = params.splittingOptions();
	m_dewarpingMode = params.dewarpingMode();
	m_depthPerception = params.depthPerception();
	m_despeckleLevel = params.despeckleLevel();
	updateDpiDisplay();
	updateColorsDisplay();
	updateDewarpingDisplay();
	splittingPanel->setChecked(m_splittingOptions.isSplitOutput());
	bwForegroundRB->setChecked(m_splittingOptions.getSplittingMode() == BLACK_AND_WHITE_FOREGROUND);
	colorForegroundRB->setChecked(m_splittingOptions.getSplittingMode() == COLOR_FOREGROUND);
	originalBackgroundCB->setChecked(m_splittingOptions.isOriginalBackgroundEnabled());

	BlackWhiteOptions::ColorSegmenterOptions const& segOpts =
		m_colorParams.blackWhiteOptions().getColorSegmenterOptions();
	colorSegmentationPanel->setChecked(segOpts.isEnabled());
	noiseReductionSB->setValue(segOpts.getNoiseReduction());
	redAdjSB->setValue(segOpts.getRedThresholdAdjustment());
	greenAdjSB->setValue(segOpts.getGreenThresholdAdjustment());
	blueAdjSB->setValue(segOpts.getBlueThresholdAdjustment());

	ColorGrayscaleOptions::PosterizationOptions const& postOpts =
		m_colorParams.colorGrayscaleOptions().getPosterizationOptions();
	posterizationPanel->setChecked(postOpts.isEnabled());
	posterizeLevelSB->setValue(postOpts.getLevel());
	posterizeNormalizeCB->setChecked(postOpts.isNormalizationEnabled());
	posterizeForceBWCB->setChecked(postOpts.isForceBlackAndWhite());
}

void
OptionsWidget::postUpdateUI()
{
}

void
OptionsWidget::tabChanged(ImageViewTab const tab)
{
	m_lastTab = tab;
	updateDpiDisplay();
	updateColorsDisplay();
	updateDewarpingDisplay();
	reloadIfNecessary();
}

void
OptionsWidget::distortionModelChanged(dewarping::DistortionModel const& model)
{
	m_ptrSettings->setDistortionModel(m_pageId, model);
	
	// Note that OFF remains OFF while AUTO becomes MANUAL.
	if (m_dewarpingMode == DewarpingMode::AUTO) {
		m_ptrSettings->setDewarpingMode(m_pageId, DewarpingMode::MANUAL);
		m_dewarpingMode = DewarpingMode::MANUAL;
		updateDewarpingDisplay();
	}
}

void
OptionsWidget::colorModeChanged(int const idx)
{
	int const mode = colorModeSelector->itemData(idx).toInt();
	m_colorParams.setColorMode((ColorParams::ColorMode)mode);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	updateColorsDisplay();
	emit reloadRequested();
}

void
OptionsWidget::whiteMarginsToggled(bool const checked)
{
	ColorGrayscaleOptions opt(m_colorParams.colorGrayscaleOptions());
	opt.setWhiteMargins(checked);
	if (!checked) {
		opt.setNormalizeIllumination(false);
		equalizeIlluminationCB->setChecked(false);
	}
	m_colorParams.setColorGrayscaleOptions(opt);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	equalizeIlluminationCB->setEnabled(checked);
	emit reloadRequested();
}

void
OptionsWidget::equalizeIlluminationToggled(bool const checked)
{
	ColorGrayscaleOptions opt(m_colorParams.colorGrayscaleOptions());
	opt.setNormalizeIllumination(checked);
	m_colorParams.setColorGrayscaleOptions(opt);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::setLighterThreshold()
{
	thresholdSlider->setValue(thresholdSlider->value() - 1);
}

void
OptionsWidget::setDarkerThreshold()
{
	thresholdSlider->setValue(thresholdSlider->value() + 1);
}

void
OptionsWidget::setNeutralThreshold()
{
	thresholdSlider->setValue(0);
}

void
OptionsWidget::bwThresholdChanged()
{
	int const value = thresholdSlider->value();
	QString const tooltip_text(QString::number(value));
	thresholdSlider->setToolTip(tooltip_text);
	
	thresholLabel->setText(QString::number(value));
	
	if (m_ignoreThresholdChanges) {
		return;
	}
	
	// Show the tooltip immediately.
	QPoint const center(thresholdSlider->rect().center());
	QPoint tooltip_pos(thresholdSlider->mapFromGlobal(QCursor::pos()));
	tooltip_pos.setY(center.y());
	tooltip_pos.setX(qBound(0, tooltip_pos.x(), thresholdSlider->width()));
	tooltip_pos = thresholdSlider->mapToGlobal(tooltip_pos);
	QToolTip::showText(tooltip_pos, tooltip_text, thresholdSlider);
	
	if (thresholdSlider->isSliderDown()) {
		// Wait for it to be released.
		// We could have just disabled tracking, but in that case we wouldn't
		// be able to show tooltips with a precise value.
		return;
	}

	BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
	if (opt.thresholdAdjustment() == value) {
		// Didn't change.
		return;
	}

	opt.setThresholdAdjustment(value);
	m_colorParams.setBlackWhiteOptions(opt);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
	
	emit invalidateThumbnail(m_pageId);
}

void
OptionsWidget::bwWhiteMarginsToggled(bool const checked)
{
	BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
	opt.setWhiteMargins(checked);
	if (!checked) {
		opt.setNormalizeIllumination(false);
		bwEqualizeIlluminationCB->setChecked(false);
	}
	m_colorParams.setBlackWhiteOptions(opt);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	bwEqualizeIlluminationCB->setEnabled(checked);
	emit reloadRequested();
}

void
OptionsWidget::bwEqualizeIlluminationToggled(bool const checked)
{
	BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
	opt.setNormalizeIllumination(checked);
	m_colorParams.setBlackWhiteOptions(opt);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::fillColorChanged(int const idx)
{
	FillingColor const color = (FillingColor)fillColorBox->itemData(idx).toInt();
	m_colorParams.setFillingColor(color);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::thresholdMethodChanged(int const idx)
{
	int const method_int = thresholdMethodBox->itemData(idx).toInt();
	BinarizationMethod const method = (BinarizationMethod)method_int;
	BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
	opt.setBinarizationMethod(method);
	m_colorParams.setBlackWhiteOptions(opt);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);

	bool const show_window = (method == SAUVOLA || method == WOLF);
	m_windowSizeLabel->setVisible(show_window);
	m_windowSizeSB->setVisible(show_window);

	emit reloadRequested();
}

void
OptionsWidget::windowSizeChanged(int const val)
{
	BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
	// Ensure odd window size
	int ws = val;
	if (ws % 2 == 0) ws++;
	opt.setWindowSize(ws);
	m_colorParams.setBlackWhiteOptions(opt);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::changeDpiButtonClicked()
{
	ChangeDpiDialog* dialog = new ChangeDpiDialog(
		this, m_outputDpi, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(
		dialog, SIGNAL(accepted(std::set<PageId> const&, Dpi const&)),
		this, SLOT(dpiChanged(std::set<PageId> const&, Dpi const&))
	);
	dialog->show();
}

void
OptionsWidget::applyColorsButtonClicked()
{
	ApplyColorsDialog* dialog = new ApplyColorsDialog(
		this, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(
		dialog, SIGNAL(accepted(std::set<PageId> const&)),
		this, SLOT(applyColorsConfirmed(std::set<PageId> const&))
	);
	dialog->show();
}

void
OptionsWidget::dpiChanged(std::set<PageId> const& pages, Dpi const& dpi)
{
	for (PageId const& page_id : pages) {
		m_ptrSettings->setDpi(page_id, dpi);
		emit invalidateThumbnail(page_id);
	}
	
	if (pages.find(m_pageId) != pages.end()) {
		m_outputDpi = dpi;
		updateDpiDisplay();
		emit reloadRequested();
	}
}

void
OptionsWidget::applyColorsConfirmed(std::set<PageId> const& pages)
{
	for (PageId const& page_id : pages) {
		m_ptrSettings->setColorParams(page_id, m_colorParams);
		emit invalidateThumbnail(page_id);
	}
	
	if (pages.find(m_pageId) != pages.end()) {
		emit reloadRequested();
	}
}

void
OptionsWidget::despeckleOffSelected()
{
	handleDespeckleLevelChange(DESPECKLE_OFF);
}

void
OptionsWidget::despeckleCautiousSelected()
{
	handleDespeckleLevelChange(DESPECKLE_CAUTIOUS);
}

void
OptionsWidget::despeckleNormalSelected()
{
	handleDespeckleLevelChange(DESPECKLE_NORMAL);
}

void
OptionsWidget::despeckleAggressiveSelected()
{
	handleDespeckleLevelChange(DESPECKLE_AGGRESSIVE);
}

void
OptionsWidget::handleDespeckleLevelChange(DespeckleLevel const level)
{
	m_despeckleLevel = level;
	m_ptrSettings->setDespeckleLevel(m_pageId, level);

	bool handled = false;
	emit despeckleLevelChanged(level, &handled);
	
	if (handled) {
		// This means we are on the "Despeckling" tab.
		emit invalidateThumbnail(m_pageId);
	} else {
		emit reloadRequested();
	}
}

void
OptionsWidget::applyDespeckleButtonClicked()
{
	ApplyColorsDialog* dialog = new ApplyColorsDialog(
		this, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowTitle(tr("Apply Despeckling Level"));
	connect(
		dialog, SIGNAL(accepted(std::set<PageId> const&)),
		this, SLOT(applyDespeckleConfirmed(std::set<PageId> const&))
	);
	dialog->show();
}

void
OptionsWidget::applyDespeckleConfirmed(std::set<PageId> const& pages)
{
	for (PageId const& page_id : pages) {
		m_ptrSettings->setDespeckleLevel(page_id, m_despeckleLevel);
		emit invalidateThumbnail(page_id);
	}
	
	if (pages.find(m_pageId) != pages.end()) {
		emit reloadRequested();
	}
}

void
OptionsWidget::changeDewarpingButtonClicked()
{
	ChangeDewarpingDialog* dialog = new ChangeDewarpingDialog(
		this, m_pageId, m_dewarpingMode, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(
		dialog, SIGNAL(accepted(std::set<PageId> const&, DewarpingMode const&)),
		this, SLOT(dewarpingChanged(std::set<PageId> const&, DewarpingMode const&))
	);
	dialog->show();
}

void
OptionsWidget::dewarpingChanged(std::set<PageId> const& pages, DewarpingMode const& mode)
{
	for (PageId const& page_id : pages) {
		m_ptrSettings->setDewarpingMode(page_id, mode);
		emit invalidateThumbnail(page_id);
	}
	
	if (pages.find(m_pageId) != pages.end()) {
		if (m_dewarpingMode != mode) {
			m_dewarpingMode = mode;
			
			// We reload when we switch to auto dewarping, even if we've just
			// switched to manual, as we don't store the auto-generated distortion model.
			// We also have to reload if we are currently on the "Fill Zones" tab,
			// as it makes use of original <-> dewarped coordinate mapping,
			// which is too hard to update without reloading.  For consistency,
			// we reload not just on TAB_FILL_ZONES but on all tabs except TAB_DEWARPING.
			// PS: the static original <-> dewarped mappings are constructed
			// in Task::UiUpdater::updateUI().  Look for "new DewarpingPointMapper" there.
			if (mode == DewarpingMode::AUTO || m_lastTab != TAB_DEWARPING) {
				// Switch to the Output tab after reloading.
				m_lastTab = TAB_OUTPUT; 

				// These depend on the value of m_lastTab.
				updateDpiDisplay();
				updateColorsDisplay();
				updateDewarpingDisplay();

				emit reloadRequested();
			} else {
				// This one we have to call anyway, as it depends on m_dewarpingMode.
				updateDewarpingDisplay();
			}
		}
	}
}

void
OptionsWidget::applyDepthPerceptionButtonClicked()
{
	ApplyColorsDialog* dialog = new ApplyColorsDialog(
		this, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowTitle(tr("Apply Depth Perception"));
	connect(
		dialog, SIGNAL(accepted(std::set<PageId> const&)),
		this, SLOT(applyDepthPerceptionConfirmed(std::set<PageId> const&))
	);
	dialog->show();
}

void
OptionsWidget::applyDepthPerceptionConfirmed(std::set<PageId> const& pages)
{
	for (PageId const& page_id : pages) {
		m_ptrSettings->setDepthPerception(page_id, m_depthPerception);
		emit invalidateThumbnail(page_id);
	}
	
	if (pages.find(m_pageId) != pages.end()) {
		emit reloadRequested();
	}
}

void
OptionsWidget::depthPerceptionChangedSlot(int val)
{
	m_depthPerception.setValue(0.1 * val);
	QString const tooltip_text(QString::number(m_depthPerception.value()));
	depthPerceptionSlider->setToolTip(tooltip_text);

	// Show the tooltip immediately.
	QPoint const center(depthPerceptionSlider->rect().center());
	QPoint tooltip_pos(depthPerceptionSlider->mapFromGlobal(QCursor::pos()));
	tooltip_pos.setY(center.y());
	tooltip_pos.setX(qBound(0, tooltip_pos.x(), depthPerceptionSlider->width()));
	tooltip_pos = depthPerceptionSlider->mapToGlobal(tooltip_pos);
	QToolTip::showText(tooltip_pos, tooltip_text, depthPerceptionSlider);

	// Propagate the signal.
	emit depthPerceptionChanged(m_depthPerception.value());
}

void
OptionsWidget::reloadIfNecessary()
{
	ZoneSet saved_picture_zones;
	ZoneSet saved_fill_zones;
	DewarpingMode saved_dewarping_mode;
	dewarping::DistortionModel saved_distortion_model;
	DepthPerception saved_depth_perception;
	DespeckleLevel saved_despeckle_level = DESPECKLE_CAUTIOUS;
	
	std::auto_ptr<OutputParams> output_params(m_ptrSettings->getOutputParams(m_pageId));
	if (output_params.get()) {
		saved_picture_zones = output_params->pictureZones();
		saved_fill_zones = output_params->fillZones();
		saved_dewarping_mode = output_params->outputImageParams().dewarpingMode();
		saved_distortion_model = output_params->outputImageParams().distortionModel();
		saved_depth_perception = output_params->outputImageParams().depthPerception();
		saved_despeckle_level = output_params->outputImageParams().despeckleLevel();
	}

	if (!PictureZoneComparator::equal(saved_picture_zones, m_ptrSettings->pictureZonesForPage(m_pageId))) {
		emit reloadRequested();
		return;
	} else if (!FillZoneComparator::equal(saved_fill_zones, m_ptrSettings->fillZonesForPage(m_pageId))) {
		emit reloadRequested();
		return;
	}

	Params const params(m_ptrSettings->getParams(m_pageId));

	if (saved_despeckle_level != params.despeckleLevel()) {
		emit reloadRequested();
		return;
	}

	if (saved_dewarping_mode == DewarpingMode::OFF && params.dewarpingMode() == DewarpingMode::OFF) {
		// In this case the following two checks don't matter.
	} else if (saved_depth_perception.value() != params.depthPerception().value()) {
		emit reloadRequested();
		return;
	} else if (saved_dewarping_mode == DewarpingMode::AUTO && params.dewarpingMode() == DewarpingMode::AUTO) {
		// The check below doesn't matter in this case.
	} else if (!saved_distortion_model.matches(params.distortionModel())) {
		emit reloadRequested();
		return;
	} else if ((saved_dewarping_mode == DewarpingMode::OFF) != (params.dewarpingMode() == DewarpingMode::OFF)) {
		emit reloadRequested();
		return;
	}
}

void
OptionsWidget::updateDpiDisplay()
{
	if (m_outputDpi.horizontal() != m_outputDpi.vertical()) {
		dpiLabel->setText(
			QString::fromLatin1("%1 x %2")
			.arg(m_outputDpi.horizontal()).arg(m_outputDpi.vertical())
		);
	} else {
		dpiLabel->setText(QString::number(m_outputDpi.horizontal()));
	}
}

void
OptionsWidget::updateColorsDisplay()
{
	colorModeSelector->blockSignals(true);
	
	ColorParams::ColorMode const color_mode = m_colorParams.colorMode();
	int const color_mode_idx = colorModeSelector->findData(color_mode);
	colorModeSelector->setCurrentIndex(color_mode_idx);
	
	bool color_grayscale_options_visible = false;
	bool bw_options_visible = false;
	switch (color_mode) {
		case ColorParams::BLACK_AND_WHITE:
			bw_options_visible = true;
			break;
		case ColorParams::COLOR_GRAYSCALE:
			color_grayscale_options_visible = true;
			break;
		case ColorParams::MIXED:
			bw_options_visible = true;
			break;
	}
	
	colorGrayscaleOptions->setVisible(color_grayscale_options_visible);
	if (color_grayscale_options_visible) {
		ColorGrayscaleOptions const opt(
			m_colorParams.colorGrayscaleOptions()
		);
		whiteMarginsCB->setChecked(opt.whiteMargins());
		equalizeIlluminationCB->setChecked(opt.normalizeIllumination());
		equalizeIlluminationCB->setEnabled(opt.whiteMargins());
	}
	
	modePanel->setVisible(m_lastTab != TAB_DEWARPING);
	bwOptions->setVisible(bw_options_visible);
	despecklePanel->setVisible(bw_options_visible && m_lastTab != TAB_DEWARPING);

	if (bw_options_visible) {
		BlackWhiteOptions const& bwOpt(m_colorParams.blackWhiteOptions());
		bwWhiteMarginsCB->setChecked(bwOpt.whiteMargins());
		bwEqualizeIlluminationCB->setChecked(bwOpt.normalizeIllumination());
		bwEqualizeIlluminationCB->setEnabled(bwOpt.whiteMargins());

		switch (m_despeckleLevel) {
			case DESPECKLE_OFF:
				despeckleOffBtn->setChecked(true);
				break;
			case DESPECKLE_CAUTIOUS:
				despeckleCautiousBtn->setChecked(true);
				break;
			case DESPECKLE_NORMAL:
				despeckleNormalBtn->setChecked(true);
				break;
			case DESPECKLE_AGGRESSIVE:
				despeckleAggressiveBtn->setChecked(true);
				break;
		}

		ScopedIncDec<int> const guard(m_ignoreThresholdChanges);
		thresholdSlider->setValue(
			m_colorParams.blackWhiteOptions().thresholdAdjustment()
		);

		int const method_idx = thresholdMethodBox->findData(
			(int)m_colorParams.blackWhiteOptions().binarizationMethod()
		);
		thresholdMethodBox->blockSignals(true);
		if (method_idx >= 0) thresholdMethodBox->setCurrentIndex(method_idx);
		thresholdMethodBox->blockSignals(false);

		BinarizationMethod const method = m_colorParams.blackWhiteOptions().binarizationMethod();
		bool const show_window = (method == SAUVOLA || method == WOLF);
		m_windowSizeLabel->setVisible(show_window);
		m_windowSizeSB->setVisible(show_window);
		m_windowSizeSB->blockSignals(true);
		m_windowSizeSB->setValue(m_colorParams.blackWhiteOptions().windowSize());
		m_windowSizeSB->blockSignals(false);
	}
	
	{
		int const fc_idx = fillColorBox->findData((int)m_colorParams.fillingColor());
		fillColorBox->blockSignals(true);
		if (fc_idx >= 0) fillColorBox->setCurrentIndex(fc_idx);
		fillColorBox->blockSignals(false);
	}

	colorModeSelector->blockSignals(false);
}

void
OptionsWidget::updateDewarpingDisplay()
{
	depthPerceptionPanel->setVisible(m_lastTab == TAB_DEWARPING);

	switch (m_dewarpingMode) {
		case DewarpingMode::OFF:
			dewarpingStatusLabel->setText(tr("Off"));
			break;
		case DewarpingMode::AUTO:
			dewarpingStatusLabel->setText(tr("Auto"));
			break;
		case DewarpingMode::MANUAL:
			dewarpingStatusLabel->setText(tr("Manual"));
			break;
	}

	depthPerceptionSlider->blockSignals(true);
	depthPerceptionSlider->setValue(qRound(m_depthPerception.value() * 10));
	depthPerceptionSlider->blockSignals(false);
}

void
OptionsWidget::colorSegmentationToggled(bool const checked)
{
	BlackWhiteOptions bwOpts(m_colorParams.blackWhiteOptions());
	BlackWhiteOptions::ColorSegmenterOptions segOpts(bwOpts.getColorSegmenterOptions());
	segOpts.setEnabled(checked);
	bwOpts.setColorSegmenterOptions(segOpts);
	m_colorParams.setBlackWhiteOptions(bwOpts);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::noiseReductionChanged(int const val)
{
	BlackWhiteOptions bwOpts(m_colorParams.blackWhiteOptions());
	BlackWhiteOptions::ColorSegmenterOptions segOpts(bwOpts.getColorSegmenterOptions());
	segOpts.setNoiseReduction(val);
	bwOpts.setColorSegmenterOptions(segOpts);
	m_colorParams.setBlackWhiteOptions(bwOpts);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::redAdjChanged(int const val)
{
	BlackWhiteOptions bwOpts(m_colorParams.blackWhiteOptions());
	BlackWhiteOptions::ColorSegmenterOptions segOpts(bwOpts.getColorSegmenterOptions());
	segOpts.setRedThresholdAdjustment(val);
	bwOpts.setColorSegmenterOptions(segOpts);
	m_colorParams.setBlackWhiteOptions(bwOpts);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::greenAdjChanged(int const val)
{
	BlackWhiteOptions bwOpts(m_colorParams.blackWhiteOptions());
	BlackWhiteOptions::ColorSegmenterOptions segOpts(bwOpts.getColorSegmenterOptions());
	segOpts.setGreenThresholdAdjustment(val);
	bwOpts.setColorSegmenterOptions(segOpts);
	m_colorParams.setBlackWhiteOptions(bwOpts);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::blueAdjChanged(int const val)
{
	BlackWhiteOptions bwOpts(m_colorParams.blackWhiteOptions());
	BlackWhiteOptions::ColorSegmenterOptions segOpts(bwOpts.getColorSegmenterOptions());
	segOpts.setBlueThresholdAdjustment(val);
	bwOpts.setColorSegmenterOptions(segOpts);
	m_colorParams.setBlackWhiteOptions(bwOpts);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::posterizationToggled(bool const checked)
{
	ColorGrayscaleOptions cgOpts(m_colorParams.colorGrayscaleOptions());
	ColorGrayscaleOptions::PosterizationOptions postOpts(cgOpts.getPosterizationOptions());
	postOpts.setEnabled(checked);
	cgOpts.setPosterizationOptions(postOpts);
	m_colorParams.setColorGrayscaleOptions(cgOpts);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::posterizeLevelChanged(int const val)
{
	ColorGrayscaleOptions cgOpts(m_colorParams.colorGrayscaleOptions());
	ColorGrayscaleOptions::PosterizationOptions postOpts(cgOpts.getPosterizationOptions());
	postOpts.setLevel(val);
	cgOpts.setPosterizationOptions(postOpts);
	m_colorParams.setColorGrayscaleOptions(cgOpts);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::posterizeNormalizeToggled(bool const checked)
{
	ColorGrayscaleOptions cgOpts(m_colorParams.colorGrayscaleOptions());
	ColorGrayscaleOptions::PosterizationOptions postOpts(cgOpts.getPosterizationOptions());
	postOpts.setNormalizationEnabled(checked);
	cgOpts.setPosterizationOptions(postOpts);
	m_colorParams.setColorGrayscaleOptions(cgOpts);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::posterizeForceBWToggled(bool const checked)
{
	ColorGrayscaleOptions cgOpts(m_colorParams.colorGrayscaleOptions());
	ColorGrayscaleOptions::PosterizationOptions postOpts(cgOpts.getPosterizationOptions());
	postOpts.setForceBlackAndWhite(checked);
	cgOpts.setPosterizationOptions(postOpts);
	m_colorParams.setColorGrayscaleOptions(cgOpts);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::splitOutputToggled(bool const checked)
{
	m_splittingOptions.setSplitOutput(checked);
	m_ptrSettings->setSplittingOptions(m_pageId, m_splittingOptions);
	emit reloadRequested();
}

void
OptionsWidget::bwForegroundToggled(bool const checked)
{
	if (!checked) return;
	m_splittingOptions.setSplittingMode(BLACK_AND_WHITE_FOREGROUND);
	m_ptrSettings->setSplittingOptions(m_pageId, m_splittingOptions);
	emit reloadRequested();
}

void
OptionsWidget::colorForegroundToggled(bool const checked)
{
	if (!checked) return;
	m_splittingOptions.setSplittingMode(COLOR_FOREGROUND);
	m_ptrSettings->setSplittingOptions(m_pageId, m_splittingOptions);
	emit reloadRequested();
}

void
OptionsWidget::originalBackgroundToggled(bool const checked)
{
	m_splittingOptions.setOriginalBackgroundEnabled(checked);
	m_ptrSettings->setSplittingOptions(m_pageId, m_splittingOptions);
	emit reloadRequested();
}

void
OptionsWidget::generatePdfToggled(bool const checked)
{
	pdfOptionsWidget->setVisible(checked);
	updatePdfSizeEstimate();
	emit generatePdfChanged(checked);
}

void
OptionsWidget::ocrLanguageToggled()
{
	QStringList langs;
	if (ocrEngCB->isChecked()) langs << "eng";
	if (ocrItaCB->isChecked()) langs << "ita";
	if (ocrFraCB->isChecked()) langs << "fra";
	if (ocrDeuCB->isChecked()) langs << "deu";
	if (ocrSpaCB->isChecked()) langs << "spa";
	if (ocrRusCB->isChecked()) langs << "rus";
	emit ocrLanguageChanged(langs.join("+"));
}

void
OptionsWidget::pdfDpiSpinChanged(int const val)
{
	updatePdfSizeEstimate();
	emit pdfDpiChanged(val);
}

void
OptionsWidget::updatePdfSizeEstimate()
{
	double const dpiVal = pdfDpiSpin->value();

	// Estimate lossless FlateDecode size for an A5 page (5.83 x 8.27 in).
	// zlib compression ratio on scanned page images: ~3-5x for text, ~2x for photos.
	// Use conservative 3x estimate.
	double const pxW   = 5.83 * dpiVal;
	double const pxH   = 8.27 * dpiVal;
	double const rawMB = pxW * pxH * 3.0 / (1024.0 * 1024.0);
	double const estMB = rawMB / 3.0 + 0.02; // +20 KB overhead per page

	QString est;
	if (estMB < 1.0)
		est = tr("~%1 KB / page (A5)").arg(int(estMB * 1024));
	else
		est = tr("~%1 MB / page (A5)").arg(estMB, 0, 'f', 1);

	// Color hint: green < 1 MB, orange 1-3 MB, red > 3 MB
	QString color = estMB < 1.0 ? "#4CAF50" : (estMB < 3.0 ? "#FF8C00" : "#CC3333");
	pdfSizeEstLabel->setText(
		QString("<small><span style='color:%1'>%2</span></small>").arg(color).arg(est));
}

} // namespace output
