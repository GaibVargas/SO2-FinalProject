// EPOS Periodic Thread Component Test Program

#include <time.h>
#include <real-time.h>

using namespace EPOS;

const unsigned int iterations = 0;
const unsigned int for_time = 30; // ms
const unsigned int period_m = 10000; // ms
const unsigned int period_h = 1000; // ms
const int n_threads = 20;

const unsigned int wcet_m = 50; // ms // ms
const unsigned int wcet_h = 50; // ms

int func_m(int id);
int func_h();

OStream cout;
Chronometer chrono;
Periodic_Thread * thread_m[n_threads]; // medium
Periodic_Thread * thread_h; // high

Mutex * mux;

int main()
{
    cout << "Testing Reorder..." << endl;

    mux = new Mutex();
    thread_h = new Periodic_Thread(RTConf(period_h * 1000, 0, 0, 0, iterations, wcet_h * 1000), &func_h);
    for (auto i = 0; i < n_threads; i++)
        thread_m[i] = new Periodic_Thread(RTConf(period_m * 1000, 0, 0, 0, iterations, wcet_m * 1000), &func_m, i);

    chrono.start();

    thread_h->join();
    for (auto i = 0; i < n_threads; i++)
        thread_m[i]->join();

    chrono.stop();

    cout << "I'm also done, bye!" << endl;

    return 0;
}

int func_m(int id)
{
    do {
        mux->lock();
        cout << "Executing medium inside mutex ID = " << id << endl;
        mux->unlock();
    } while (Periodic_Thread::wait_next());
    cout << "Medium finish ID = " << id << endl;
    return 0;
}

int func_h()
{
    do {
        mux->lock();
        Microsecond elapsed = chrono.read() / 1000;
        for(Microsecond end = elapsed + for_time, last = end; end > elapsed; elapsed = chrono.read() / 1000)
            if(last != elapsed) {
                if (elapsed%3 == 0)
                    cout << "Executing high inside mutex" << endl;
                last = elapsed;
            }
        cout << "High releases mutex" << endl;
        mux->unlock();
    } while (Periodic_Thread::wait_next());
    cout << "High finish" << endl;
    return 0;
}
