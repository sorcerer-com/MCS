// Thread.h
#pragma once

#include <map>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>


namespace MyEngine {

	using lock = lock_guard < _Mutex_base > ;

	struct Thread
	{
	private:
		atomic_bool interrupt;
		vector<thread> workers;
		map<string, mutex> mutices;
		map<string, recursive_mutex> recursive_mutices;

	public:
		Thread()
		{
			this->interrupt = false;
		}

		template <class Fn, class... Args>
		inline void defWorker(Fn&& fn, Args&&... args)
		{
			this->workers.push_back(thread(fn, args...));
		}

		inline thread& worker(int idx)
		{
			if (idx >= this->workers.size())
				throw "Try to access invalid worker";

			return this->workers[idx];
		}

		inline size_t workersCount()
		{
			return this->workers.size();
		}

		inline bool interrupted()
		{
			return this->interrupt;
		}

		inline void join(bool itr = true)
		{
			this->interrupt = itr;

			if (itr)
			{
				for (auto& worker : this->workers)
					if (worker.joinable())
						worker.join();
			}
		}


		inline void defMutex(const string& name, bool recursive = false)
		{
			if (!recursive)
				this->mutices[name];
			else
				this->recursive_mutices[name];
		}

		inline _Mutex_base& mutex(const string& name)
		{
			if (this->mutices.find(name) != this->mutices.end())
				return this->mutices[name];
			if (this->recursive_mutices.find(name) != this->recursive_mutices.end())
				return this->recursive_mutices[name];

			throw "Try to access invalid mutex";
		}

	};

}