//
// The MIT License(MIT)
// 
// Copyright(c) 2015 WoLfulus (wolfulus@gmail.com)
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once

#include <vector>
#include <hash_map>
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <ctime>
#include <chrono>

namespace timer
{
	///
	/// Timer id
	///
	typedef unsigned long long timer_id;

	///
	/// Timer 
	///
	static const timer_id invalid_timer = -1;

	///
	/// Types
	///
	typedef std::chrono::steady_clock clock;
	typedef clock::duration duration;
	typedef clock::time_point time;

	///
	/// Callback
	///
	typedef std::function<void()> callback;

	///
	/// Task
	///
	class timer
	{
		///
		/// Scheduler access
		///
		friend class manager;

	protected:
		/// Timer id
		timer_id id;
		
		/// Handler
		callback handler;

		/// Current time
		time last;

		/// Last execution time
		time next;

		/// Interval
		duration interval;

		/// Count
		int count;

		/// Step
		int step;

		///
		/// Constructor
		///
		timer(callback handler, duration interval, int count) :
			handler(handler), count(count), interval(interval)
		{
			this->next = clock::now() + interval;
		}

		///
		/// Executes the handler and schedules next run
		///
		void execute()
		{
			this->handler();
			this->next += this->interval;
		}

		///
		/// Executes the task
		///
		bool update()
		{
			if (!this->handler)
			{
				return true;
			}

			auto now = clock::now();
			if (this->next <= now)
			{
				if (this->count < 0)
				{
					this->execute();
					return true;
				}
				else if (this->count == 0)
				{
					while (this->next <= now)
					{
						this->execute();
					}
				}
				else if (this->count > 1)
				{
					while (this->next <= now)
					{
						this->execute();
						this->count -= 1;
						if (this->count == 1)
						{
							return true;
						}
					}
				}
				else
				{
					return true;
				}
			}

			return false;
		}
	};

	///
	/// Scheduler
	///
	class manager
	{
	private:
		///
		/// Timers
		///
		timer_id ids;

		///
		/// List of tasks
		///
		std::hash_map<timer_id, timer*> timers;

		///
		/// Worker thread
		///
		std::thread worker;

		///
		/// Mutex
		///
		std::recursive_mutex mutex;

		///
		/// Delay
		///
		int delay;

		///
		/// Stopped flag
		///
		bool stopped;

		///
		/// List of timers to be removed
		///
		std::vector<timer_id> remove;

	public:
		///
		/// Constructor
		///
		manager::manager()
			: ids(0)
		{
		}

		///
		/// Destructor
		///
		manager::~manager()
		{
			this->stop();
		}

		///
		/// Clears the manager
		///
		void clear()
		{
			std::lock_guard<std::recursive_mutex> lock(this->mutex);
			this->ids = 0;
			this->timers.clear();
		}

		///
		/// Creates a timeout task
		///
		template<typename T>
		timer_id timeout(callback handler, T dur)
		{
			std::lock_guard<std::recursive_mutex> lock(this->mutex);
			auto id = this->get_id();
			timer* t = new timer(handler, std::chrono::duration_cast<duration>(dur), -1);
			this->timers[id] = t;
			return id;
		}

		///
		/// Creates a timeout task
		///
		template<typename T>
		timer_id interval(callback handler, T dur)
		{
			std::lock_guard<std::recursive_mutex> lock(this->mutex);
			auto id = this->get_id();
			timer* t = new timer(handler, std::chrono::duration_cast<duration>(dur), 0);
			this->timers[id] = t;
			return id;
		}

		///
		/// Creates a timeout task
		///
		template<typename T>
		timer_id repeat(callback handler, T dur, int count)
		{
			if (count <= 0)
			{
				return ::timer::invalid_timer;
			}
			else if (count == 1)
			{
				return this->timeout(handler, dur);
			}

			std::lock_guard<std::recursive_mutex> lock(this->mutex);
			auto id = this->get_id();
			timer* t = new timer(handler, std::chrono::duration_cast<duration>(dur), count + 1);
			this->timers[id] = t;
			return id;
		}

		///
		/// Cancel a timer
		///
		void cancel(timer_id id)
		{
			std::lock_guard<std::recursive_mutex> lock(this->mutex);
			if (id != ::timer::invalid_timer)
			{
				this->remove.push_back(id);
			}
		}

		///
		/// Starts the update thread
		///
		void start()
		{
			this->start(std::chrono::milliseconds(250));
		}

		///
		/// Starts the update thread
		///
		template<typename T>
		void start(T dur)
		{
			this->stopped = false;
			this->worker = std::thread(std::bind(&manager::update_loop, this, std::chrono::duration_cast<duration>(dur)));
		}

		///
		/// Stops the worker
		///
		void stop()
		{
			this->stopped = true;
			if (this->worker.joinable())
			{
				this->worker.join();
			}
		}

		///
		/// Updates the scheduler (manually)
		///
		void update()
		{
			std::lock_guard<std::recursive_mutex> lock(this->mutex);

			auto it = this->timers.begin();
			while (it != this->timers.end())
			{
				auto now = clock::now();
				if (it->second->update())
				{
					this->remove.push_back(it->first);
				}
				it++;
			}

			for (auto r : this->remove)
			{
				it = this->timers.find(r);
				if (it != this->timers.end())
				{
					delete it->second;
					this->timers.erase(it);
				}
			}

			this->remove.clear();
		}

		///
		/// Joins the worker
		///
		void wait()
		{
			if (this->worker.joinable())
			{
				this->worker.join();
			}
		}

	protected:
		///
		/// Id generation
		///
		timer_id get_id()
		{
			std::lock_guard<std::recursive_mutex> lock(this->mutex);
			timer_id id = this->ids;
			do
			{
				id = this->ids++;
				if (id == ::timer::invalid_timer)
				{
					continue;
				}
				if (this->timers.find(id) == this->timers.end())
				{
					break;
				}
			} while (true);

			return id;
		}

		///
		/// Update loop
		///
		void update_loop(duration delay)
		{
			auto zero = std::chrono::milliseconds(0);
			while (!this->stopped)
			{
				auto start = clock::now();
				this->update();
				auto end = clock::now() - start;
				auto diff = delay - end;
				if (diff > zero)
				{
					std::this_thread::sleep_for(diff);
				}
			}
		}
	};
}