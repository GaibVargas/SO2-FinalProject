// EPOS Periodic Thread Component Test Program

#include <time.h>
#include <real-time.h>

using namespace EPOS;

const unsigned int iterations = 1;
const unsigned int for_time_low1 = 30; // ms
const unsigned int for_time_low2 = 20; // ms
const unsigned int for_time_high = 50; // ms
const unsigned int period_l = 2 * for_time_high + 100; // ms
const unsigned int period_m = 2 * for_time_high + 100; // ms
const unsigned int period_h = 2 * for_time_high + 100; // ms

const unsigned int wcet_l = 2 * for_time_high + 50; // ms
const unsigned int wcet_m = 2 * for_time_high + 100; // ms // ms
const unsigned int wcet_h = 2 * for_time_high + 150; // ms

int func_l0();
int func_l1();
int func_l2();
int func_m();
int func_h();
long max(unsigned int a, unsigned int b, unsigned int c) { return ((a >= b) && (a >= c)) ? a : ((b >= a) && (b >= c) ? b : c); }

OStream cout;
Chronometer chrono;
Periodic_Thread * thread_l0; // low0
Periodic_Thread * thread_l1; // low1
Periodic_Thread * thread_l2; // low2
Periodic_Thread * thread_m; // medium
Periodic_Thread * thread_h; // high

Semaphore * sem1;
Semaphore * sem2;

int main()
{
    cout << "Testing PCP..." << endl;

    sem1 = new Semaphore(4);
    sem2 = new Semaphore(3);
    thread_l0 = new Periodic_Thread(RTConf(period_l * 1000, 0, 0, 0, iterations, wcet_l * 1000), &func_l0);

    chrono.start();

    thread_l0->join();
    thread_l1->join();
    thread_l2->join();
    thread_m->join();
    thread_h->join();

    chrono.stop();

    cout << "I'm also done, bye!" << endl;

    return 0;
}

int func_l0()
{
    do {
        cout << "Executing low0" << endl;
        cout << "Executing low0" << endl;
        cout << "Low0 tries to get semaphore 1" << endl;
        sem1->p();
        cout << "Low0 gets semaphore 1" << endl;
        cout << "Low0 tries to get semaphore 2" << endl;
        sem2->p();
        cout << "Low0 gets semaphore 2" << endl;
        if (!thread_l1) {
            cout << "Create thread Low1" << endl;
            thread_l1 = new Periodic_Thread(RTConf(period_l * 1000, 0, 0, 0, iterations, wcet_h * 1000), &func_l1);
        }
        Microsecond elapsed1 = chrono.read() / 1000;
        for(Microsecond end = elapsed1 + for_time_low1, last = end; end > elapsed1; elapsed1 = chrono.read() / 1000)
            if(last != elapsed1) {
                if (elapsed1%2 == 0)
                    cout << "Executing low0 inside sem2 p(l) = " << thread_l0->priority() << endl;
                last = elapsed1;
            }

        cout << "Low0 releases semaphore 2" << endl;
        sem2->v();

        Microsecond elapsed2 = chrono.read() / 1000;
        for(Microsecond end = elapsed2 + for_time_low1, last = end; end > elapsed2; elapsed2 = chrono.read() / 1000)
            if(last != elapsed2) {
                if (elapsed2%2 == 0)
                    cout << "Executing low0 inside sem1 p(l) = " << thread_l0->priority() << endl;
                last = elapsed2;
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
        cout << "Low1 tries to get semaphore 2" << endl;
        sem2->p();
        cout << "Low1 gets semaphore 2" << endl;
        if (!thread_l2) {
            cout << "Create thread Low2" << endl;
            thread_l2 = new Periodic_Thread(RTConf(period_l * 1000, 0, 0, 0, iterations, wcet_h * 1000), &func_l2);
        }
        Microsecond elapsed1 = chrono.read() / 1000;
        for(Microsecond end = elapsed1 + for_time_low1, last = end; end > elapsed1; elapsed1 = chrono.read() / 1000)
            if(last != elapsed1) {
                if (elapsed1%3 == 0)
                    cout << "Executing low1 inside sem2 p(l) = " << thread_l1->priority() << endl;
                last = elapsed1;
            }

        cout << "Low1 releases semaphore 2" << endl;
        sem2->v();

        Microsecond elapsed2 = chrono.read() / 1000;
        for(Microsecond end = elapsed2 + for_time_low1, last = end; end > elapsed2; elapsed2 = chrono.read() / 1000)
            if(last != elapsed2) {
                if (elapsed2%3 == 0)
                    cout << "Executing low1 inside sem1 p(l) = " << thread_l1->priority() << endl;
                last = elapsed2;
            }

        cout << "Low1 releases semaphore 1" << endl;
        sem1->v();
        cout << "Executing low1 p(l) = " << thread_l1->priority() << endl;
        cout << "Low1 done" << endl;
    } while (Periodic_Thread::wait_next());
    cout << "Low1 finish" << endl;
    return 0;
}

int func_l2()
{
    do {
        cout << "Executing low2" << endl;
        cout << "Executing low2" << endl;
        cout << "Low2 tries to get semaphore 1" << endl;
        sem1->p();
        cout << "Low2 gets semaphore 1" << endl;
        cout << "Low2 tries to get semaphore 2" << endl;
        sem2->p();
        cout << "Low2 gets semaphore 2" << endl;
        if (!thread_h) {
            cout << "Create thread High" << endl;
            thread_h = new Periodic_Thread(RTConf(period_h * 1000, 0, 0, 0, iterations, wcet_h * 1000), &func_h);
        }

        Microsecond elapsed1 = chrono.read() / 1000;
        for(Microsecond end = elapsed1 + for_time_low2, last = end; end > elapsed1; elapsed1 = chrono.read() / 1000)
            if(last != elapsed1) {
                if (elapsed1%5 == 0)
                    cout << "Executing low2 inside sem2 p(l) = " << thread_l2->priority() << endl;
                last = elapsed1;
            }

        cout << "Low2 releases semaphore 2" << endl;
        sem2->v();

        Microsecond elapsed2 = chrono.read() / 1000;
        for(Microsecond end = elapsed2 + for_time_low2, last = end; end > elapsed2; elapsed2 = chrono.read() / 1000)
            if(last != elapsed2) {
                if (elapsed2%5 == 0)
                    cout << "Executing low2 inside sem1 p(l) = " << thread_l2->priority() << endl;
                last = elapsed2;
            }

        sem1->v();
        cout << "Executing low2 p(l) = " << thread_l2->priority() << endl;
        cout << "Low2 done" << endl;
    } while (Periodic_Thread::wait_next());
    cout << "Low2 finish" << endl;
    return 0;
}

int func_m()
{
    do {
        cout << "Executing medium" << endl;
        cout << "Medium tries to get semaphore 1" << endl;
        sem1->p();
        cout << "Medium gets semaphore 1" << endl;
        cout << "Executing medium inside semaphore 1" << endl;
        cout << "Medium releases semaphore 1" << endl;
        sem1->v();
        cout << "Medium done" << endl;
    } while (Periodic_Thread::wait_next());
    cout << "Medium finish" << endl;
    return 0;
}

int func_h()
{
    do {
        cout << "Executing high" << endl;
        cout << "High tries to get semaphore 1" << endl;
        sem1->p();
        if (!thread_m) {
            cout << "Create thread Medium" << endl;
            thread_m = new Periodic_Thread(RTConf(period_m * 1000, 0, 0, 0, iterations, wcet_m * 1000), &func_m);
        }
        cout << "High gets semaphore 1" << endl;
        cout << "Executing high inside semaphore 1" << endl;
        cout << "High tries to get semaphore 2" << endl;
        sem2->p();
        cout << "High gets semaphore 2" << endl;
        Microsecond elapsed = chrono.read() / 1000;
        for(Microsecond end = elapsed + for_time_high, last = end; end > elapsed; elapsed = chrono.read() / 1000)
            if(last != elapsed) {
                if (elapsed%7 == 0)
                    cout << "Executing high inside sem2 p(h) = " << thread_h->priority() << endl;
                last = elapsed;
            }
        cout << "Executing high inside semaphore 2" << endl;
        cout << "High releases semaphore 2" << endl;
        sem2->v();
        cout << "High releases semaphore 1" << endl;
        sem1->v();
        cout << "High done" << endl;
    } while (Periodic_Thread::wait_next());
    cout << "High finish" << endl;
    return 0;
}
