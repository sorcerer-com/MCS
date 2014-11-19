// Thread.h
#pragma once

#include <map>
#include <thread>
#include <atomic>
#include <mutex>


namespace Engine {

	struct Thread
	{
	private:
		atomic_bool m_interrupt;
		thread m_worker;
		map<string, mutex> m_mutices;

	public:
		Thread()
		{
			this->m_interrupt = false;
		}

		template <class Fn, class... Args>
		Thread(Fn&& fn, Args&&... args) : Thread()
		{
			this->worker(fn, args);
		}

		template <class Fn, class... Args>
		void worker(Fn&& fn, Args&&... args)
		{
			this->m_worker = thread(fn, args...);
		}

		bool interrupted()
		{
			return this->m_interrupt;
		}

		void join(bool itr = true)
		{
			this->m_interrupt = itr;

			if (itr && this->m_worker.joinable())
				this->m_worker.join();
		}

		void defMutex(const string& name)
		{
			this->m_mutices[name];
		}

		mutex& mutex(const string& name)
		{
			if (this->m_mutices.find(name) == this->m_mutices.end())
				throw "Try to access invalid mutex";
			
			return this->m_mutices[name];
		}

	};

}