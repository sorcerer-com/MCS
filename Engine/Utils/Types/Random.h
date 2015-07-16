// Random.h
#pragma once

#include <vector>
#include <thread>
#include <random>


namespace MyEngine {

    struct Random
    {
    private:
        std::mt19937 generator; // mersenne twister generator

    public:
        Random(unsigned seed = 123u)
        {
            this->seed(seed);
        }

        inline void seed(unsigned seed)
        {
            generator.seed(seed);
        }

        // returns a raw 32-bit unbiased random integer
        inline unsigned _next(void) 
        {
            std::uniform_int_distribution<unsigned> gen;
            return gen(generator);
        }

        // returns a random integer in [a..b] (a and b can be negative as well)
        inline int randInt(int a, int b)
        {
            std::uniform_int_distribution<int> gen(a, b);
            return gen(generator);
        }

        // return a floating-point number in [0..1)
        inline float randFloat(void)
        {
            std::uniform_real_distribution<float> gen;
            return gen(generator);
        }

        inline float randSample(int samples)
        {
            if (samples > 0)
                return (float)(this->_next() % samples + this->randFloat()) / samples;
            else
                return this->randFloat();
        }

        inline float randSample(int numSamples, int sample)
        {
            if (numSamples > 0)
                return (sample % numSamples + this->randFloat()) / numSamples;
            else
                return this->randFloat();
        }

        // return a random number in normal distribution
        inline double gaussian(double mean = 0.0, double sigma = 1.0)
        {
            std::normal_distribution<double> gen(mean, sigma);
            return gen(generator);
        }

        // get a random point in the unit disc (x*x + y*y <= 1)
        inline void unitDiscSample(float& x, float &y)
        {	
            // pick a random point in the unit disc with uniform probability by using polar coords.
            // Note the sqrt(). For explanation why it's needed, see 
            // http://mathworld.wolfram.com/DiskPointPicking.html
            float angle = randFloat() * 2.0f * 3.14159265359f;
            float rad = sqrt(randFloat());
            x = sinf(angle) * rad;
            y = cosf(angle) * rad;
        }

    private:
        static const int RGENS = 257; // 257 is a prime number
        static pair<unsigned, Random> rg_table[RGENS];

    public:
        // seed the whole array of random generators.
        static inline void initRandom(unsigned seed)
        {
            for (int i = 0; i < RGENS; i++)
                rg_table[i].first = 0xffffffff;

            const int MAXWARM = 1223;
            seed ^= 0xbf14ef80; // just in case the user passes '0'...
            // initialize and warm-up the zeroth random generator:
            rg_table[0].second.seed(seed);
            for (int i = 0; i < MAXWARM; i++) 
                rg_table[0].second._next();
            for (int i = 1; i < RGENS; i++)
            {
                Random& prev = rg_table[i - 1].second;
                Random& next = rg_table[i].second;
                next.seed(prev._next());
                int n = prev.randInt(0, MAXWARM - 1);
                for (int i = 0; i < n; i++)
                    next._next();
            }
        }

        // fetch the idx-th random generator. There are at least 250 random generators, which are prepared and ready.
        // This function does not take any start-up time and should be very fast.
        static inline Random& getRandomGen(int idx)
        {
            unsigned key = idx;
            int i = ((unsigned)idx % (unsigned)RGENS);
            for (int k = 0; k < RGENS; k++)
            {
                if (rg_table[i].first == key)
                    return rg_table[i].second;
                else if (rg_table[i].first == 0xffffffff)
                {
                    rg_table[i].first = key;
                    return rg_table[i].second;
                }
                else
                {
                    i++;
                    if (i >= RGENS) i -= RGENS;
                }
            }
            return rg_table[i].second;
        }

        // fetch a fixed random generator, based on the calling thread's ID. I.e., within each thread, all calls to getRandomGen()
        // are guaranteed to return the same object; in the same time, different threads get different random generators
        // thus no locking is required, and no performance degradation can occur.
        static inline Random& getRandomGen(void)
        {
            return getRandomGen((int)this_thread::get_id().hash());
        }

    };

}