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
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <vector>
#include <atomic>

#include "PageId.h"
#include "ImageId.h"
#include "filters/deskew/Settings.h"
#include "filters/deskew/Params.h"
#include "filters/deskew/Dependencies.h"
#include "filters/output/Settings.h"
#include "filters/output/Params.h"
#include "AutoManualMode.h"
#include "Dpi.h"

namespace {

PageId makePageId(int index)
{
	return PageId(
		ImageId(QString("test_image_%1.jpg").arg(index), 0),
		PageId::SINGLE_PAGE
	);
}

class DeskewWriterThread : public QThread
{
public:
	DeskewWriterThread(deskew::Settings& settings, int start, int count)
		: m_settings(settings), m_start(start), m_count(count) {}

	void run() override
	{
		deskew::Dependencies deps;
		for (int i = 0; i < m_count; ++i) {
			int idx = m_start + (i % 50);
			double angle = (i % 90) - 45.0;
			deskew::Params params(angle, deps, (i % 2) ? MODE_AUTO : MODE_MANUAL);
			m_settings.setPageParams(makePageId(idx), params);
		}
	}
private:
	deskew::Settings& m_settings;
	int m_start;
	int m_count;
};

class DeskewReaderThread : public QThread
{
public:
	DeskewReaderThread(deskew::Settings& settings, int count,
	                   std::atomic<int>& found)
		: m_settings(settings), m_count(count), m_found(found) {}

	void run() override
	{
		for (int i = 0; i < m_count; ++i) {
			int idx = i % 50;
			auto params = m_settings.getPageParams(makePageId(idx));
			if (params) {
				m_found.fetch_add(1, std::memory_order_relaxed);
				// Verify returned data is valid
				double angle = params->deskewAngle();
				BOOST_CHECK(angle >= -45.0 && angle <= 45.0);
			}
		}
	}
private:
	deskew::Settings& m_settings;
	int m_count;
	std::atomic<int>& m_found;
};

class OutputWriterThread : public QThread
{
public:
	OutputWriterThread(output::Settings& settings, int start, int count)
		: m_settings(settings), m_start(start), m_count(count) {}

	void run() override
	{
		for (int i = 0; i < m_count; ++i) {
			int idx = m_start + (i % 50);
			PageId pid = makePageId(idx);

			// Mix different partial-update methods
			switch (i % 4) {
			case 0:
				m_settings.setDpi(pid, Dpi(300 + (i % 4) * 100, 300 + (i % 4) * 100));
				break;
			case 1: {
				output::ColorParams cp;
				cp.setColorMode(static_cast<output::ColorParams::ColorMode>(i % 3));
				m_settings.setColorParams(pid, cp);
				break;
			}
			case 2:
				m_settings.setDespeckleLevel(pid,
					static_cast<output::DespeckleLevel>(i % 4)
				);
				break;
			case 3:
				m_settings.setDewarpingMode(pid,
					output::DewarpingMode(static_cast<output::DewarpingMode::Mode>(i % 3))
				);
				break;
			}
		}
	}
private:
	output::Settings& m_settings;
	int m_start;
	int m_count;
};

class OutputReaderThread : public QThread
{
public:
	OutputReaderThread(output::Settings& settings, int count,
	                   std::atomic<int>& reads)
		: m_settings(settings), m_count(count), m_reads(reads) {}

	void run() override
	{
		for (int i = 0; i < m_count; ++i) {
			int idx = i % 50;
			output::Params params = m_settings.getParams(makePageId(idx));
			m_reads.fetch_add(1, std::memory_order_relaxed);
			// Verify DPI values are sane
			BOOST_CHECK(params.outputDpi().horizontal() >= 0);
			BOOST_CHECK(params.outputDpi().vertical() >= 0);
		}
	}
private:
	output::Settings& m_settings;
	int m_count;
	std::atomic<int>& m_reads;
};

} // anonymous namespace


BOOST_AUTO_TEST_SUITE(SettingsThreadSafety)

BOOST_AUTO_TEST_CASE(deskew_concurrent_write_read)
{
	deskew::Settings settings;
	std::atomic<int> found(0);
	int const ops_per_thread = 500;

	DeskewWriterThread w1(settings, 0, ops_per_thread);
	DeskewWriterThread w2(settings, 25, ops_per_thread);
	DeskewReaderThread r1(settings, ops_per_thread, found);
	DeskewReaderThread r2(settings, ops_per_thread, found);

	w1.start();
	w2.start();
	r1.start();
	r2.start();

	w1.wait();
	w2.wait();
	r1.wait();
	r2.wait();

	// Verify some reads succeeded (writes were happening concurrently)
	BOOST_CHECK(found.load() > 0);
}

BOOST_AUTO_TEST_CASE(deskew_concurrent_write_clear)
{
	deskew::Settings settings;
	int const ops = 200;

	// Pre-populate
	deskew::Dependencies deps;
	for (int i = 0; i < 50; ++i) {
		settings.setPageParams(makePageId(i),
			deskew::Params(1.0, deps, MODE_AUTO));
	}

	// Writer thread sets params while main thread clears
	DeskewWriterThread writer(settings, 0, ops);
	writer.start();

	for (int i = 0; i < 10; ++i) {
		settings.clear();
	}

	writer.wait();
	// No crash = success
	BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(output_concurrent_partial_updates)
{
	output::Settings settings;
	std::atomic<int> reads(0);
	int const ops_per_thread = 500;

	OutputWriterThread w1(settings, 0, ops_per_thread);
	OutputWriterThread w2(settings, 25, ops_per_thread);
	OutputReaderThread r1(settings, ops_per_thread, reads);
	OutputReaderThread r2(settings, ops_per_thread, reads);

	w1.start();
	w2.start();
	r1.start();
	r2.start();

	w1.wait();
	w2.wait();
	r1.wait();
	r2.wait();

	BOOST_CHECK(reads.load() == ops_per_thread * 2);
}

BOOST_AUTO_TEST_CASE(output_concurrent_full_params_set)
{
	output::Settings settings;
	int const num_threads = 4;
	int const ops = 200;

	std::vector<std::unique_ptr<QThread>> threads;
	for (int t = 0; t < num_threads; ++t) {
		class FullParamsWriter : public QThread {
		public:
			FullParamsWriter(output::Settings& s, int start, int count)
				: m_s(s), m_start(start), m_count(count) {}
			void run() override {
				for (int i = 0; i < m_count; ++i) {
					output::Params p;
					p.setOutputDpi(Dpi(300, 300));
					p.setDespeckleLevel(output::DESPECKLE_NORMAL);
					m_s.setParams(makePageId(m_start + (i % 20)), p);
				}
			}
		private:
			output::Settings& m_s;
			int m_start, m_count;
		};
		auto thread = std::make_unique<FullParamsWriter>(settings, t * 20, ops);
		thread->start();
		threads.push_back(std::move(thread));
	}

	for (auto& t : threads) {
		t->wait();
	}

	// Verify data integrity after concurrent writes
	for (int i = 0; i < 80; ++i) {
		output::Params p = settings.getParams(makePageId(i));
		BOOST_CHECK(p.outputDpi() == Dpi(300, 300));
		BOOST_CHECK(p.despeckleLevel() == output::DESPECKLE_NORMAL);
	}
}

BOOST_AUTO_TEST_SUITE_END()
