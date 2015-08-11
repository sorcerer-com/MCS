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

    enum mutex_type
    {
        normal,
        recursive,
        read_write
    };

    struct rw_mutex
    {
    private:
        mutex mtx;
        condition_variable gate1;
        condition_variable gate2;
        unsigned state;

        static const unsigned write_entered = 1U << (sizeof(unsigned) * CHAR_BIT - 1);
        static const unsigned n_readers = ~write_entered;

    public:
        rw_mutex()
        {
            state = 0;
        }

        inline void read_lock()
        {
            unique_lock<mutex> lck(mtx);

            while ((state & write_entered) || (state & n_readers) == n_readers)
                gate1.wait(lck);
            unsigned num_readers = (state & n_readers) + 1;
            state &= ~n_readers;
            state |= num_readers;
        }

        inline void read_unlock()
        {
            lock_guard<mutex> _(mtx);

            unsigned num_readers = (state & n_readers) - 1;
            state &= ~n_readers;
            state |= num_readers;
            if (state & write_entered)
            {
                if (num_readers == 0)
                    gate2.notify_one();
            }
            else
            {
                if (num_readers == n_readers - 1)
                    gate1.notify_one();
            }
        }

        inline void write_lock()
        {
            unique_lock<mutex> lck(mtx);

            while (state & write_entered)
                gate1.wait(lck);
            state |= write_entered;
            while (state & n_readers)
                gate2.wait(lck);
        }

        inline void write_unlock()
        {
            lock_guard<mutex> _(mtx);

            state = 0;
            gate1.notify_all();
        }
    };

    
	struct Thread
	{
	private:
		atomic_bool interrupt;
		vector<thread> workers;
		map<string, mutex> mutices;
        map<string, recursive_mutex> recursive_mutices;
        map<string, rw_mutex> rw_mutices;

        queue<packaged_task<bool(int)>> tasks;
        mutex tasksMutex;
        atomic_int waitCounter;

	public:
		Thread()
		{
            this->interrupt = false;
            this->waitCounter = 0;
		}

        template <typename Fn, class... Args>
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


		inline void defMutex(const string& name, mutex_type type = mutex_type::normal)
		{
			if (type == mutex_type::normal)
				this->mutices[name];
            else if (type == mutex_type::recursive)
                this->recursive_mutices[name];
            else if (type == mutex_type::read_write)
                this->rw_mutices[name];
		}

		inline _Mutex_base& mutex(const string& name)
		{
			if (this->mutices.find(name) != this->mutices.end())
				return this->mutices[name];
			if (this->recursive_mutices.find(name) != this->recursive_mutices.end())
				return this->recursive_mutices[name];

			throw "Try to access invalid mutex";
        }

        inline rw_mutex& rw_mutex(const string& name)
        {
            if (this->rw_mutices.find(name) != this->rw_mutices.end())
                return this->rw_mutices[name];

            throw "Try to access invalid mutex";
        }


        inline void defThreadPool(int threadsCount = 0)
        {
            if (threadsCount == 0)
            {
                threadsCount = thread::hardware_concurrency();
                if (threadsCount > 1)
                    threadsCount--;
            }

            this->workers.clear();
            for (int i = 0; i < threadsCount; i++)
                this->defWorker(&Thread::doTask, this, i);
        }

        inline future<bool> addTask(const function<bool(int)>& func)
        {
            lock lck(this->tasksMutex);
            this->tasks.push(packaged_task<bool(int)>(func));
            return this->tasks.back().get_future();
        }

        inline void addNTasks(const function<bool(int)>& func, int num = 0)
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