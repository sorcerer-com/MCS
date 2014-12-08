// Thread.h
#pragma once

#include <map>
#include <thread>
#include <atomic>
#include <mutex>


namespace MyEngine {

	using lock = lock_guard < _Mutex_base > ;

	struct Thread
	{
	private:
		atomic_bool m_interrupt;
		thread m_worker;
		map<string, mutex> m_mutices;
		map<string, recursive_mutex> m_recursive_mutices;

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
		inline void worker(Fn&& fn, Args&&... args)
		{
			this->m_worker = thread(fn, args...);
		}

		inline bool interrupted()
		{
			return this->m_interrupt;
		}

		inline void join(bool itr = true)
		{
			this->m_interrupt = itr;

			if (itr && this->m_worker.joinable())
				this->m_worker.join();
		}


		inline void defMutex(const string& name, bool recursive = false)
		{
			if (!recursive)
				this->m_mutices[name];
			else
				this->m_recursive_mutices[name];
		}

		inline _Mutex_base& mutex(const string& name)
		{
			if (this->m_mutices.find(name) != this->m_mutices.end())
				return this->m_mutices[name];
			if (this->m_recursive_mutices.find(name) != this->m_recursive_mutices.end())
				return this->m_recursive_mutices[name];
			
			throw "Try to access invalid mutex";
		}

	};

}