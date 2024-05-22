// EPOS Periodic Thread Component Test Program

#include <time.h>
#include <real-time.h>

using namespace EPOS;

const unsigned int iterations = 0;
const unsigned int for_time = 30; // ms
const unsigned int period_l = 100000; // ms
const unsigned int period_m = 10000; // ms
const unsigned int period_h = 1000; // ms

const unsigned int wcet_l = 50; // ms
const unsigned int wcet_m = 50; // ms // ms
const unsigned int wcet_h = 50; // ms

int func_l0();
int func_l1();
int func_m(int id);
int func_h();
long max(unsigned int a, unsigned int b, unsigned int c) { return ((a >= b) && (a >= c)) ? a : ((b >= a) && (b >= c) ? b : c); }

OStream cout;
Chronometer chrono;
Periodic_Thread * thread_l0; // low0
Periodic_Thread * thread_l1; // low1
Periodic_Thread * thread_m[4]; // medium
Periodic_Thread * thread_h; // high

Semaphore * sem1;

int main()
{
    cout << "Testing Reorder..." << endl;

    sem1 = new Semaphore(2);
    thread_l1 = new Periodic_Thread(RTConf(period_l * 1000, 0, 0, 0, iterations, wcet_l * 1000), &func_l1);
    thread_l0 = new Periodic_Thread(RTConf(period_l * 1000, 0, 0, 0, iterations, wcet_l * 1000), &func_l0);

    chrono.start();

    thread_l0->join();
    thread_l1->join();
    for (auto i = 0; i < 4; i++)
        thread_m[i]->join();
    thread_h->join();

    chrono.stop();

    cout << "I'm also done, bye!" << endl;

    return 0;
}

int func_l0()
{
    do {
        cout << "Executing low0" << endl;
        cout << "Low0 tries to get semaphore 1" << endl;
        sem1->p();
        cout << "Low0 gets semaphore 1" << endl;
        cout << "Creates medium ID = 0" << endl;
    
        thread_m[0] = new Periodic_Thread(RTConf(period_m * 1000, 0, 0, 0, iterations, (wcet_m + 100) * 1000), &func_m, 0);

        Microsecond elapsed1 = chrono.read() / 1000;
        for(Microsecond end = elapsed1 + for_time*3, last = end; end > elapsed1; elapsed1 = chrono.read() / 1000)
            if(last != elapsed1) {
                if (elapsed1%17 == 0)
                    cout << "Executing low0 inside sem1 p(l) = " << thread_l0->priority() << endl;
                last = elapsed1;
            }

        cout << "Low0 releases semaphore 1" << endl;
        sem1->v();
        cout << "Executing low0 p(l) = " << thread_l0->priority() << endl;
        cout << "Low0 done" << endl;
    } while (Periodic_Thread::wait_next());
    cout << "Low0 finish" << endl;
    return 0;
}

int func_l1()
{
    do {
        cout << "Executing low1" << endl;
        cout << "Executing low1" << endl;
        cout << "Low1 tries to get semaphore 1" << endl;
        sem1->p();
        cout << "Low1 gets semaphore 1" << endl;
        Microsecond elapsed = chrono.read() / 1000;
        for(Microsecond end = elapsed + for_time*3, last = end; end > elapsed; elapsed = chrono.read() / 1000)
            if(last != elapsed) {
                if (elapsed % 13 == 0)
                    cout << "Executing low1 inside sem1 p(l) = " << thread_l1->priority() << endl;
                last = elapsed;
            }
        cout << "Low1 releases semaphore 1" << endl;
        sem1->v();
        cout << "Executing low1 p(l) = " << thread_l1->priority() << endl;
        cout << "Low1 done" << endl;
    } while (Periodic_Thread::wait_next());
    cout << "Low1 finish" << endl;
    return 0;
}

int func_m(int id)
{
    int primos[4] = {3 ,5, 7, 11};
    do {
        cout << "Executing medium ID = " << id << endl;
        if (id == 0) {
            for (auto i = 1; i < 4; i++) {
                cout << "Creates medium ID = " << i << endl;
                thread_m[i] = new Periodic_Thread(RTConf(period_m * 1000, 0, 0, 0, iterations, wcet_m * 1000), &func_m, i);
            }
            thread_h = new Periodic_Thread(RTConf(period_h * 1000, 0, 0, 0, iterations, wcet_h * 1000), &func_h);
        }

        cout << "Executing medium ID = " << id << endl;
        Microsecond elapsed1 = chrono.read() / 1000;
        for(Microsecond end = elapsed1 + for_time, last = end; end > elapsed1; elapsed1 = chrono.read() / 1000)
            if(last != elapsed1) {
                if (elapsed1%primos[id] == 0)
                    cout << "Executing medium ID = " << id << endl;
                last = elapsed1;
            }
        cout << "Medium done" << endl;
    } while (Periodic_Thread::wait_next());
    cout << "Medium finish ID = " << id << endl;
    return 0;
}

int func_h()
{
    do {
        cout << "Executing high" << endl;
        cout << "High tries to get semaphore 1" << endl;
        sem1->p();
        cout << "High gets semaphore 1" << endl;
        cout << "Executing high inside semaphore 1" << endl;
        Microsecond elapsed = chrono.read() / 1000;
        for(Microsecond end = elapsed + for_time, last = end; end > elapsed; elapsed = chrono.read() / 1000)
            if(last != elapsed) {
                if (elapsed%13 == 0)
                    cout << "Executing high inside sem1" << endl;
                last = elapsed;
            }
        cout << "High releases semaphore 1" << endl;
        sem1->v();
        cout << "High done" << endl;
    } while (Periodic_Thread::wait_next());
    cout << "High finish" << endl;
    return 0;
}
