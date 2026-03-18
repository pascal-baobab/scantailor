/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) Joseph Artsimovich <joseph.artsimovich@gmail.com>
    Copyright (C) 2019 4lex4 <4lex49@zoho.com>

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

#include "WorkerThreadPool.h"

#include <QCoreApplication>
#include <QThread>
#include <QThreadPool>
#include <algorithm>
#include <utility>

#include "OutOfMemoryHandler.h"

class WorkerThreadPool::TaskResultEvent : public QEvent
{
public:
	TaskResultEvent(BackgroundTaskPtr task, FilterResultPtr result)
		: QEvent(User), m_task(std::move(task)), m_result(std::move(result)) {}

	const BackgroundTaskPtr& task() const { return m_task; }
	const FilterResultPtr& result() const { return m_result; }

private:
	BackgroundTaskPtr m_task;
	FilterResultPtr m_result;
};


WorkerThreadPool::WorkerThreadPool(QObject* parent)
	: QObject(parent), m_pool(new QThreadPool(this))
{
	updateNumberOfThreads();
}

WorkerThreadPool::~WorkerThreadPool() = default;

void WorkerThreadPool::shutdown()
{
	m_pool->waitForDone();
}

bool WorkerThreadPool::hasSpareCapacity() const
{
	return m_pool->activeThreadCount() < m_pool->maxThreadCount();
}

void WorkerThreadPool::submitTask(BackgroundTaskPtr const& task)
{
	class Runnable : public QRunnable
	{
	public:
		Runnable(WorkerThreadPool& owner, BackgroundTaskPtr task)
			: m_owner(owner), m_task(std::move(task))
		{
			setAutoDelete(true);
		}

		void run() override
		{
			if (m_task->isCancelled()) {
				return;
			}
			try {
				FilterResultPtr const result((*m_task)());
				if (result) {
					QCoreApplication::postEvent(
						&m_owner, new TaskResultEvent(m_task, result)
					);
				}
			} catch (std::bad_alloc const&) {
				OutOfMemoryHandler::instance().handleOutOfMemorySituation();
			}
		}

	private:
		WorkerThreadPool& m_owner;
		BackgroundTaskPtr m_task;
	};

	updateNumberOfThreads();
	m_pool->start(new Runnable(*this, task));
}

void WorkerThreadPool::customEvent(QEvent* event)
{
	if (auto* evt = dynamic_cast<TaskResultEvent*>(event)) {
		emit taskResult(evt->task(), evt->result());
	}
}

void WorkerThreadPool::refreshSettings()
{
	m_threadCountCached = false;
	updateNumberOfThreads();
}

void WorkerThreadPool::updateNumberOfThreads()
{
	if (m_threadCountCached) {
		return;
	}

	int maxThreads = QThread::idealThreadCount();
	// Restrict to 2 threads on 32-bit due to address space constraints.
	if (sizeof(void*) <= 4) {
		maxThreads = std::min(maxThreads, 2);
	}

	int numThreads = m_settings.value(
		"settings/batch_processing_threads", maxThreads
	).toInt();
	numThreads = std::min(numThreads, maxThreads);
	numThreads = std::max(numThreads, 1);
	m_pool->setMaxThreadCount(numThreads);
	m_threadCountCached = true;
}
