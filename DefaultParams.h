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

#ifndef DEFAULTPARAMS_H_
#define DEFAULTPARAMS_H_

#include "Dpi.h"
#include "Margins.h"
#include "OrthogonalRotation.h"
#include "AutoManualMode.h"
#include "filters/output/ColorParams.h"
#include "filters/output/SplittingOptions.h"
#include "filters/output/DepthPerception.h"
#include "filters/output/DespeckleLevel.h"
#include "filters/page_layout/Alignment.h"
#include "filters/page_split/LayoutType.h"
#include <QSizeF>
#include <QString>

class QDomDocument;
class QDomElement;

class DefaultParams
{
public:
	class FixOrientationParams
	{
	public:
		FixOrientationParams();
		explicit FixOrientationParams(OrthogonalRotation const& image_rotation);
		explicit FixOrientationParams(QDomElement const& el);
		QDomElement toXml(QDomDocument& doc, QString const& name) const;

		OrthogonalRotation const& getImageRotation() const { return m_imageRotation; }
		void setImageRotation(OrthogonalRotation const& rotation) { m_imageRotation = rotation; }
	private:
		OrthogonalRotation m_imageRotation;
	};

	class DeskewParams
	{
	public:
		DeskewParams();
		DeskewParams(double deskew_angle_deg, AutoManualMode mode);
		explicit DeskewParams(QDomElement const& el);
		QDomElement toXml(QDomDocument& doc, QString const& name) const;

		double getDeskewAngleDeg() const { return m_deskewAngleDeg; }
		void setDeskewAngleDeg(double angle) { m_deskewAngleDeg = angle; }
		AutoManualMode getMode() const { return m_mode; }
		void setMode(AutoManualMode mode) { m_mode = mode; }
	private:
		double m_deskewAngleDeg;
		AutoManualMode m_mode;
	};

	class PageSplitParams
	{
	public:
		PageSplitParams();
		explicit PageSplitParams(page_split::LayoutType layout_type);
		explicit PageSplitParams(QDomElement const& el);
		QDomElement toXml(QDomDocument& doc, QString const& name) const;

		page_split::LayoutType getLayoutType() const { return m_layoutType; }
		void setLayoutType(page_split::LayoutType type) { m_layoutType = type; }
	private:
		page_split::LayoutType m_layoutType;
	};

	class SelectContentParams
	{
	public:
		SelectContentParams();
		SelectContentParams(QSizeF const& page_rect_size, bool content_detect_enabled, bool fine_tune_corners);
		explicit SelectContentParams(QDomElement const& el);
		QDomElement toXml(QDomDocument& doc, QString const& name) const;

		QSizeF const& getPageRectSize() const { return m_pageRectSize; }
		void setPageRectSize(QSizeF const& size) { m_pageRectSize = size; }
		bool isContentDetectEnabled() const { return m_contentDetectEnabled; }
		void setContentDetectEnabled(bool enabled) { m_contentDetectEnabled = enabled; }
		bool isFineTuneCorners() const { return m_fineTuneCorners; }
		void setFineTuneCorners(bool fine_tune) { m_fineTuneCorners = fine_tune; }
	private:
		QSizeF m_pageRectSize;
		bool m_contentDetectEnabled;
		bool m_fineTuneCorners;
	};

	class PageLayoutParams
	{
	public:
		PageLayoutParams();
		PageLayoutParams(Margins const& hard_margins, page_layout::Alignment const& alignment);
		explicit PageLayoutParams(QDomElement const& el);
		QDomElement toXml(QDomDocument& doc, QString const& name) const;

		Margins const& getHardMargins() const { return m_hardMargins; }
		void setHardMargins(Margins const& margins) { m_hardMargins = margins; }
		page_layout::Alignment const& getAlignment() const { return m_alignment; }
		void setAlignment(page_layout::Alignment const& alignment) { m_alignment = alignment; }
	private:
		Margins m_hardMargins;
		page_layout::Alignment m_alignment;
	};

	class OutputParams
	{
	public:
		OutputParams();
		OutputParams(Dpi const& dpi, output::ColorParams const& color_params,
			output::SplittingOptions const& splitting_options,
			output::DepthPerception const& depth_perception,
			double despeckle_level);
		explicit OutputParams(QDomElement const& el);
		QDomElement toXml(QDomDocument& doc, QString const& name) const;

		Dpi const& getDpi() const { return m_dpi; }
		void setDpi(Dpi const& dpi) { m_dpi = dpi; }
		output::ColorParams const& getColorParams() const { return m_colorParams; }
		void setColorParams(output::ColorParams const& params) { m_colorParams = params; }
		output::SplittingOptions const& getSplittingOptions() const { return m_splittingOptions; }
		void setSplittingOptions(output::SplittingOptions const& opts) { m_splittingOptions = opts; }
		output::DepthPerception const& getDepthPerception() const { return m_depthPerception; }
		void setDepthPerception(output::DepthPerception const& dp) { m_depthPerception = dp; }
		double getDespeckleLevel() const { return m_despeckleLevel; }
		void setDespeckleLevel(double level) { m_despeckleLevel = level; }
	private:
		Dpi m_dpi;
		output::ColorParams m_colorParams;
		output::SplittingOptions m_splittingOptions;
		output::DepthPerception m_depthPerception;
		double m_despeckleLevel;
	};

public:
	DefaultParams();

	DefaultParams(FixOrientationParams const& fix_orientation,
		DeskewParams const& deskew,
		PageSplitParams const& page_split,
		SelectContentParams const& select_content,
		PageLayoutParams const& page_layout,
		OutputParams const& output);

	explicit DefaultParams(QDomElement const& el);

	QDomElement toXml(QDomDocument& doc, QString const& name) const;

	FixOrientationParams const& getFixOrientationParams() const { return m_fixOrientationParams; }
	void setFixOrientationParams(FixOrientationParams const& params) { m_fixOrientationParams = params; }

	DeskewParams const& getDeskewParams() const { return m_deskewParams; }
	void setDeskewParams(DeskewParams const& params) { m_deskewParams = params; }

	PageSplitParams const& getPageSplitParams() const { return m_pageSplitParams; }
	void setPageSplitParams(PageSplitParams const& params) { m_pageSplitParams = params; }

	SelectContentParams const& getSelectContentParams() const { return m_selectContentParams; }
	void setSelectContentParams(SelectContentParams const& params) { m_selectContentParams = params; }

	PageLayoutParams const& getPageLayoutParams() const { return m_pageLayoutParams; }
	void setPageLayoutParams(PageLayoutParams const& params) { m_pageLayoutParams = params; }

	OutputParams const& getOutputParams() const { return m_outputParams; }
	void setOutputParams(OutputParams const& params) { m_outputParams = params; }

private:
	FixOrientationParams m_fixOrientationParams;
	DeskewParams m_deskewParams;
	PageSplitParams m_pageSplitParams;
	SelectContentParams m_selectContentParams;
	PageLayoutParams m_pageLayoutParams;
	OutputParams m_outputParams;
};

#endif
