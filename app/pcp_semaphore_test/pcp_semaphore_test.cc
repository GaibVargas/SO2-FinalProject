// EPOS Periodic Thread Component Test Program

#include <time.h>
#include <real-time.h>

using namespace EPOS;

const unsigned int iterations = 1;
const unsigned int period_l = 200; // ms
const unsigned int period_ml = 175; // ms
const unsigned int period_m = 150; // ms
const unsigned int period_h = 100; // ms

const unsigned int wcet_l = 10; // ms
const unsigned int wcet_ml = 15; // ms
const unsigned int wcet_m = 20; // ms
const unsigned int wcet_h = 30; // ms

int func_l();
int func_ml();
int func_m();
int func_h();
long max(unsigned int a, unsigned int b, unsigned int c) { return ((a >= b) && (a >= c)) ? a : ((b >= a) && (b >= c) ? b : c); }

OStream cout;
Chronometer chrono;
Periodic_Thread * thread_l; // low
Periodic_Thread * thread_ml; // medium low
Periodic_Thread * thread_m; // medium
Periodic_Thread * thread_h; // high

Semaphore * sem1;
Semaphore * sem2;

int main()
{
    cout << "Testing PCP..." << endl;

    sem1 = new Semaphore(2);
    sem2 = new Semaphore(1);
    thread_l = new Periodic_Thread(RTConf(period_l * 1000, 0, 0, 0, iterations, wcet_l * 1000), &func_l);

    chrono.start();

    thread_l->join();
    thread_ml->join();
    thread_m->join();
    thread_h->join();

    chrono.stop();

    cout << "I'm also done, bye!" << endl;

    return 0;
}

int func_l()
{
    do {
        cout << "Executing low" << endl;
        cout << "Executing low" << endl;
        cout << "Low tries to get semaphore 1" << endl;
        sem1->p();
        cout << "Low gets semaphore 1" << endl;
        cout << "Low tries to get semaphore 2" << endl;
        sem2->p();
        cout << "Low gets semaphore 2" << endl;
        if (!thread_m) {
            cout << "Create thread Medium" << endl;
            thread_m = new Periodic_Thread(RTConf(period_m * 1000, 0, 0, 0, iterations, wcet_h * 1000), &func_m);
        }
        cout << "Executing low p(l) = " << thread_l->priority() << endl;
        if (!thread_h) {
            cout << "Create thread High" << endl;
            thread_h = new Periodic_Thread(RTConf(period_h * 1000, 0, 0, 0, iterations, wcet_h * 1000), &func_h);
        }
        cout << "Executing low inside semaphore 2" << endl;
        cout << "Executing low p(l) = " << thread_l->priority() << endl;
        cout << "Low releases semaphore 2" << endl;
        sem2->v();
        cout << "Executing low p(l) = " << thread_l->priority() << endl;
        cout << "Low releases semaphore 1" << endl;
        sem1->v();
        cout << "Executing low p(l) = " << thread_l->priority() << endl;
        cout << "Low done" << endl;
    } while (Periodic_Thread::wait_next());
    cout << "Low finish" << endl;
    return 0;
}

int func_m()
{
    do {
        cout << "Executing medium" << endl;
        cout << "Medium tries to get semaphore 1" << endl;
        sem1->p();
        cout << "Medium gets semaphore 1" << endl;
        cout << "Medium tries to get semaphore 2" << endl;
        sem2->p();
        cout << "Medium gets semaphore 2" << endl;
        cout << "Executing medium inside semaphore 2" << endl;
        if (!thread_ml) {
            cout << "Create thread Medium Low" << endl;
            thread_ml = new Periodic_Thread(RTConf(period_ml * 1000, 0, 0, 0, iterations, wcet_ml * 1000), &func_ml);
        }
        cout << "Medium releases semaphore 2" << endl;
        sem2->v();
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
        // if (!thread_ml) {
        //     cout << "Create thread Medium Low" << endl;
        //     thread_ml = new Periodic_Thread(RTConf(period_ml * 1000, 0, 0, 0, iterations, wcet_ml * 1000), &func_ml);
        // }
        cout << "High tries to get semaphore 2" << endl;
        sem2->p();
        cout << "High gets semaphore 2" << endl;
        cout << "Executing high inside semaphore 2" << endl;
        cout << "High releases semaphore 2" << endl;
        sem2->v();
        cout << "High done" << endl;
    } while (Periodic_Thread::wait_next());
    cout << "High finish" << endl;
    return 0;
}

int func_ml()
{
    do {
        cout << "Executing Medium Low " << endl;
        cout << "Medium Low done " << endl;
    } while (Periodic_Thread::wait_next());
    cout << "Medium Low finish" << endl;
    return 0;
}