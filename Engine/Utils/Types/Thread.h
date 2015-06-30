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

        queue<packaged_task<bool(int)>> tasks;
        mutex tasksMutex;
        atomic_int waitCounter;

	public:
		Thread()
		{
            this->interrupt = false;
            this->waitCounter = 0;
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

        inline void iterruptWorkers(bool itr = true)
        {
            this->interrupt = itr;
        }

		inline bool interrupted()
		{
			return this->interrupt;
		}

		inline void joinWorkers()
		{
            this->iterruptWorkers();

            for (auto& worker : this->workers)
				if (worker.joinable())
					worker.join();
            this->workers.clear();

            this->iterruptWorkers(false);
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
            {
                threadsCount = std::thread::hardware_concurrency();
                if (threadsCount > 1)
                    threadsCount--;
            }

            for (int i = 0; i < threadsCount; i++)
                this->defWorker(&Thread::doTask, this, i);
        }

        inline future<bool> addTask(function<bool(int)> func)
        {
            lock lck(this->tasksMutex);
            this->tasks.push(packaged_task<bool(int)>(func));
            return this->tasks.back().get_future();
        }

        inline void addNTasks(function<bool(int)> func, int num = 0)
        {
            if (num == 0)
                num = (int)this->workers.size();

            for (int i = 0; i < num; i++)
                this->addTask(func);
        }

        inline void addWaitTask()
        {
            this->addNTasks([&](int id) { return this->waitTask(id); });
        }

    private:
        void doTask(int id)
        {
            while (!this->interrupt)
            {
                while (!this->tasks.empty())
                {
                    packaged_task<bool(int)> task;
                    {
                        lock lck(this->tasksMutex);
                        if (this->tasks.empty())
                            continue;
                        task = move(this->tasks.front());
                        this->tasks.pop();
                    }

                    task(id);

                    this_thread::sleep_for(chrono::milliseconds(1));
                }

                this_thread::sleep_for(chrono::milliseconds(100));
            }
        }

        bool waitTask(int id)
        {
            for (int i = 0; i < this->workers.size(); i++)
                this->waitCounter |= (2 << i);

            while (!this->interrupt && this->waitCounter != 0)
            {
                this_thread::sleep_for(chrono::milliseconds(100));
                this->waitCounter &= ~(2 << id); // clear 2 pow id
            }
            return true;
        }

	};

}