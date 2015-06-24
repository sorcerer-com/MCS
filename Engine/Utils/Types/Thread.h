// Thread.h
#pragma once

#include <map>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>


namespace MyEngine {

	using lock = lock_guard < _Mutex_base > ;

	struct Thread
	{
	private:
		atomic_bool interrupt;
		vector<thread> workers;
		map<string, mutex> mutices;
		map<string, recursive_mutex> recursive_mutices;

        queue<packaged_task<bool()>> tasks;
        mutex tasksMutex;

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
			if (idx >= (int)this->workers.size())
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
                this->workers.clear();
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


        inline void defThreadPool(int threadsCount = 0)
        {
            if (threadsCount == 0)
                threadsCount = std::thread::hardware_concurrency();

            for (int i = 0; i < threadsCount; i++)
                this->defWorker(&Thread::doTask, this);
        }

        inline future<bool> addTask(function<bool()> func)
        {
            lock lck(this->tasksMutex);
            this->tasks.push(packaged_task<bool()>(func));
            return this->tasks.back().get_future();
        }

        inline void addNTasks(function<bool()> func, int num)
        {
            for (int i = 0; i < num; i++)
                this->addTask(func);
        }

    private:
        void doTask()
        {
            while (!this->interrupt)
            {
                while (!this->tasks.empty())
                {
                    packaged_task<bool()> task;
                    {
                        lock lck(this->tasksMutex);
                        if (this->tasks.empty())
                            continue;
                        task = move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task();

                    this_thread::sleep_for(chrono::milliseconds(1));
                }

                this_thread::sleep_for(chrono::milliseconds(100));
            }
        }

	};

}