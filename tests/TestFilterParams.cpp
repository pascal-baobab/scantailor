/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

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

#include <boost/test/auto_unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <QDomDocument>
#include <QDomElement>
#include <QRectF>
#include <QSizeF>
#include <QPolygonF>
#include <QPointF>
#include <cmath>

#include "filters/deskew/Params.h"
#include "filters/deskew/Dependencies.h"
#include "filters/page_layout/Params.h"
#include "filters/page_layout/Alignment.h"
#include "filters/select_content/Params.h"
#include "filters/select_content/Dependencies.h"
#include "filters/output/Params.h"
#include "filters/output/ColorParams.h"
#include "filters/output/SplittingOptions.h"
#include "filters/output/DewarpingMode.h"
#include "filters/output/DespeckleLevel.h"
#include "filters/output/DepthPerception.h"
#include "Margins.h"
#include "Dpi.h"
#include "OrthogonalRotation.h"
#include "AutoManualMode.h"

BOOST_AUTO_TEST_SUITE(FilterParamsRoundTrip)

//===================== deskew::Params =====================

BOOST_AUTO_TEST_CASE(deskew_params_round_trip_auto)
{
	QPolygonF outline;
	outline << QPointF(0, 0) << QPointF(100, 0)
	        << QPointF(100, 200) << QPointF(0, 200);
	OrthogonalRotation rotation;
	deskew::Dependencies deps(outline, rotation);
	deskew::Params original(2.75, deps, MODE_AUTO);

	QDomDocument doc;
	QDomElement el = original.toXml(doc, "deskew-params");
	doc.appendChild(el);

	deskew::Params restored(el);
	BOOST_REQUIRE_CLOSE(restored.deskewAngle(), 2.75, 0.001);
	BOOST_CHECK(restored.mode() == MODE_AUTO);
}

BOOST_AUTO_TEST_CASE(deskew_params_round_trip_manual)
{
	deskew::Dependencies deps;
	deskew::Params original(-1.5, deps, MODE_MANUAL);

	QDomDocument doc;
	QDomElement el = original.toXml(doc, "deskew-params");
	doc.appendChild(el);

	deskew::Params restored(el);
	BOOST_REQUIRE_CLOSE(restored.deskewAngle(), -1.5, 0.001);
	BOOST_CHECK(restored.mode() == MODE_MANUAL);
}

BOOST_AUTO_TEST_CASE(deskew_params_zero_angle)
{
	deskew::Dependencies deps;
	deskew::Params original(0.0, deps, MODE_AUTO);

	QDomDocument doc;
	QDomElement el = original.toXml(doc, "deskew-params");
	doc.appendChild(el);

	deskew::Params restored(el);
	BOOST_CHECK(std::abs(restored.deskewAngle()) < 0.0001);
}

//===================== page_layout::Params =====================

BOOST_AUTO_TEST_CASE(page_layout_params_round_trip)
{
	Margins margins(10.0, 5.0, 10.0, 5.0);
	QSizeF content_size(150.0, 200.0);
	page_layout::Alignment alignment(
		page_layout::Alignment::VCENTER,
		page_layout::Alignment::HCENTER
	);
	page_layout::Params original(margins, content_size, alignment);

	QDomDocument doc;
	QDomElement el = original.toXml(doc, "page-layout-params");
	doc.appendChild(el);

	page_layout::Params restored(el);
	BOOST_REQUIRE_CLOSE(restored.hardMarginsMM().top(), 5.0, 0.001);
	BOOST_REQUIRE_CLOSE(restored.hardMarginsMM().bottom(), 5.0, 0.001);
	BOOST_REQUIRE_CLOSE(restored.hardMarginsMM().left(), 10.0, 0.001);
	BOOST_REQUIRE_CLOSE(restored.hardMarginsMM().right(), 10.0, 0.001);
	BOOST_REQUIRE_CLOSE(restored.contentSizeMM().width(), 150.0, 0.001);
	BOOST_REQUIRE_CLOSE(restored.contentSizeMM().height(), 200.0, 0.001);
	BOOST_CHECK(restored.alignment() == alignment);
}

BOOST_AUTO_TEST_CASE(page_layout_params_asymmetric_margins)
{
	Margins margins(5.0, 10.0, 15.0, 20.0);
	QSizeF content_size(80.5, 120.3);
	page_layout::Alignment alignment(
		page_layout::Alignment::TOP,
		page_layout::Alignment::LEFT
	);
	page_layout::Params original(margins, content_size, alignment);

	QDomDocument doc;
	QDomElement el = original.toXml(doc, "page-layout-params");
	doc.appendChild(el);

	page_layout::Params restored(el);
	BOOST_REQUIRE_CLOSE(restored.hardMarginsMM().left(), 5.0, 0.001);
	BOOST_REQUIRE_CLOSE(restored.hardMarginsMM().top(), 10.0, 0.001);
	BOOST_REQUIRE_CLOSE(restored.hardMarginsMM().right(), 15.0, 0.001);
	BOOST_REQUIRE_CLOSE(restored.hardMarginsMM().bottom(), 20.0, 0.001);
	BOOST_CHECK(restored.alignment().vertical() == page_layout::Alignment::TOP);
	BOOST_CHECK(restored.alignment().horizontal() == page_layout::Alignment::LEFT);
}

//===================== select_content::Params =====================

BOOST_AUTO_TEST_CASE(select_content_params_round_trip)
{
	QRectF content_rect(10.0, 20.0, 300.0, 400.0);
	QSizeF content_size_mm(100.0, 150.0);
	QRectF page_rect(0.0, 0.0, 320.0, 440.0);
	select_content::Dependencies deps;
	select_content::Params original(
		content_rect, content_size_mm, deps,
		MODE_MANUAL, page_rect, MODE_AUTO
	);

	QDomDocument doc;
	QDomElement el = original.toXml(doc, "select-content-params");
	doc.appendChild(el);

	select_content::Params restored(el);
	BOOST_REQUIRE_CLOSE(restored.contentRect().x(), 10.0, 0.001);
	BOOST_REQUIRE_CLOSE(restored.contentRect().y(), 20.0, 0.001);
	BOOST_REQUIRE_CLOSE(restored.contentRect().width(), 300.0, 0.001);
	BOOST_REQUIRE_CLOSE(restored.contentRect().height(), 400.0, 0.001);
	BOOST_REQUIRE_CLOSE(restored.contentSizeMM().width(), 100.0, 0.001);
	BOOST_REQUIRE_CLOSE(restored.contentSizeMM().height(), 150.0, 0.001);
	BOOST_CHECK(restored.mode() == MODE_MANUAL);
	BOOST_CHECK(restored.pageDetectMode() == MODE_AUTO);
}

BOOST_AUTO_TEST_CASE(select_content_params_no_page_rect)
{
	QRectF content_rect(50.0, 50.0, 200.0, 300.0);
	QSizeF content_size_mm(80.0, 120.0);
	select_content::Dependencies deps;
	select_content::Params original(
		content_rect, content_size_mm, deps, MODE_AUTO
	);

	QDomDocument doc;
	QDomElement el = original.toXml(doc, "select-content-params");
	doc.appendChild(el);

	select_content::Params restored(el);
	BOOST_REQUIRE_CLOSE(restored.contentRect().x(), 50.0, 0.001);
	BOOST_REQUIRE_CLOSE(restored.contentRect().y(), 50.0, 0.001);
	BOOST_CHECK(restored.mode() == MODE_AUTO);
	BOOST_CHECK(restored.pageDetectMode() == MODE_DISABLED);
}

//===================== output::Params =====================

BOOST_AUTO_TEST_CASE(output_params_round_trip_bw)
{
	output::Params original;
	original.setOutputDpi(Dpi(600, 600));
	output::ColorParams cp;
	cp.setColorMode(output::ColorParams::BLACK_AND_WHITE);
	original.setColorParams(cp);
	original.setDewarpingMode(output::DewarpingMode(output::DewarpingMode::OFF));
	original.setDespeckleLevel(output::DESPECKLE_NORMAL);
	original.setDepthPerception(output::DepthPerception(2.0));

	QDomDocument doc;
	QDomElement el = original.toXml(doc, "output-params");
	doc.appendChild(el);

	output::Params restored(el);
	BOOST_CHECK(restored.outputDpi() == Dpi(600, 600));
	BOOST_CHECK(restored.colorParams().colorMode() == output::ColorParams::BLACK_AND_WHITE);
	BOOST_CHECK(restored.dewarpingMode() == output::DewarpingMode::OFF);
	BOOST_CHECK(restored.despeckleLevel() == output::DESPECKLE_NORMAL);
	BOOST_REQUIRE_CLOSE(restored.depthPerception().value(), 2.0, 0.001);
}

BOOST_AUTO_TEST_CASE(output_params_round_trip_mixed)
{
	output::Params original;
	original.setOutputDpi(Dpi(300, 300));
	output::ColorParams cp;
	cp.setColorMode(output::ColorParams::MIXED);
	cp.setFillingColor(output::FILL_BACKGROUND);
	original.setColorParams(cp);
	original.setDewarpingMode(output::DewarpingMode(output::DewarpingMode::AUTO));
	original.setDespeckleLevel(output::DESPECKLE_AGGRESSIVE);
	original.setDepthPerception(output::DepthPerception(2.5));

	output::SplittingOptions split_opts;
	split_opts.setSplitOutput(true);
	split_opts.setSplittingMode(output::COLOR_FOREGROUND);
	split_opts.setOriginalBackgroundEnabled(true);
	original.setSplittingOptions(split_opts);

	QDomDocument doc;
	QDomElement el = original.toXml(doc, "output-params");
	doc.appendChild(el);

	output::Params restored(el);
	BOOST_CHECK(restored.outputDpi() == Dpi(300, 300));
	BOOST_CHECK(restored.colorParams().colorMode() == output::ColorParams::MIXED);
	BOOST_CHECK(restored.dewarpingMode() == output::DewarpingMode::AUTO);
	BOOST_CHECK(restored.despeckleLevel() == output::DESPECKLE_AGGRESSIVE);
	BOOST_REQUIRE_CLOSE(restored.depthPerception().value(), 2.5, 0.001);
	BOOST_CHECK(restored.splittingOptions() == split_opts);
}

BOOST_AUTO_TEST_CASE(output_params_round_trip_color)
{
	output::Params original;
	original.setOutputDpi(Dpi(150, 150));
	output::ColorParams cp;
	cp.setColorMode(output::ColorParams::COLOR_GRAYSCALE);
	original.setColorParams(cp);
	original.setDewarpingMode(output::DewarpingMode(output::DewarpingMode::MANUAL));
	original.setDespeckleLevel(output::DESPECKLE_OFF);

	QDomDocument doc;
	QDomElement el = original.toXml(doc, "output-params");
	doc.appendChild(el);

	output::Params restored(el);
	BOOST_CHECK(restored.outputDpi() == Dpi(150, 150));
	BOOST_CHECK(restored.colorParams().colorMode() == output::ColorParams::COLOR_GRAYSCALE);
	BOOST_CHECK(restored.dewarpingMode() == output::DewarpingMode::MANUAL);
	BOOST_CHECK(restored.despeckleLevel() == output::DESPECKLE_OFF);
}

BOOST_AUTO_TEST_SUITE_END()
