// Image.h
#pragma once

#include <chrono>
#include <mutex>

#include "..\..\Engine.h"


namespace MyEngine {

#define Profile Profiler prof(__FUNCTION__)
#define ProfileLog Profiler prof(__FUNCTION__, true)

    string duration_to_string(chrono::system_clock::duration delta);

    struct Profiler
    {
    private:
        string name;
        bool log;
        chrono::system_clock::time_point time;

    public:
        Profiler()
        {
            if (Engine::Mode == EngineMode::EEngine)
                return;

            this->name = "";
            this->log = false;
        }

        Profiler(const string& name, bool log = false)
        {
            if (Engine::Mode == EngineMode::EEngine)
                return;

            this->name = name;
            this->log = log;
            this->start();
        }

        ~Profiler()
        {
            lock_guard<mutex> lck(Profiler::dataMutex);
            if (Engine::Mode == EngineMode::EEngine || this->name == "")
                return;

            chrono::system_clock::duration delta = this->stop();
            Profiler::data[this->name].deltaSum += delta;
            Profiler::data[this->name].counter++;

            if (this->log)
                Engine::Log(LogType::ELog, "Profiler", this->name + " (" + duration_to_string(delta) + ")");
        }


        inline void start()
        {
            this->time = chrono::system_clock::now();
        }

        inline chrono::system_clock::duration stop()
        {
            if (this->time == chrono::system_clock::time_point())
                return chrono::system_clock::duration();

            chrono::system_clock::duration delta = chrono::system_clock::now() - this->time;
            this->time = chrono::system_clock::now();

            return delta;
        }

        inline chrono::system_clock::duration duration()
        {
            if (this->time == chrono::system_clock::time_point())
                return chrono::system_clock::duration();

            return chrono::system_clock::now() - this->time;
        }

    private:
        struct Data
        {
            chrono::system_clock::duration deltaSum;
            int counter;

            Data()
            {
                deltaSum = chrono::system_clock::duration();
                counter = 0;
            }
        };

        static map<string, Data> data;
        static mutex dataMutex;

    public:
        static map<string, long long> GetDurations()
        {
            lock_guard<mutex> lck(Profiler::dataMutex);
            map<string, long long> durations;
            for (auto& d : data)
            {
                long long milisecs = chrono::duration_cast<chrono::milliseconds>(d.second.deltaSum).count();
                durations[d.first] = milisecs / d.second.counter;
            }
            return durations;
        }
    };
    
    inline string duration_to_string(chrono::system_clock::duration delta)
    {
        int hours = chrono::duration_cast<chrono::hours>(delta).count();
        int minutes = chrono::duration_cast<chrono::minutes>(delta).count();
        long long seconds = chrono::duration_cast<chrono::seconds>(delta).count();
        long long milisecs = chrono::duration_cast<chrono::milliseconds>(delta).count();
        milisecs -= seconds * 1000;
        seconds -= minutes * 60;
        minutes -= hours * 60;
        return to_string(hours) + "h " + to_string(minutes) + "min " + to_string(seconds) + "sec " + to_string(milisecs) + "milisec";
    }
}