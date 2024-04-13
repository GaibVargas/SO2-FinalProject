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

Semaphore * sem;

int main()
{
    cout << "Testing PCP..." << endl;

    sem = new Semaphore(2);
    thread_l = new Periodic_Thread(RTConf(period_l * 1000, 0, 0, 0, iterations, wcet_l * 1000), &func_l);

    chrono.start();

    thread_l->join();
    thread_m->join();
    thread_h->join();

    chrono.stop();

    cout << "I'm also done, bye!" << endl;

    return 0;
}

int func_l()
{
    cout << "Executing low" << endl;
    cout << "Executing low" << endl;
    cout << "Low tries to get semaphore" << endl;
    sem->p();
    cout << "Low gets semaphore" << endl;
    cout << "Create thread Medium" << endl;
    thread_m = new Periodic_Thread(RTConf(period_m * 1000, 0, 0, 0, iterations, wcet_h * 1000), &func_m);
    cout << "Executing low inside semaphore " << endl;
    cout << "Low releases semaphore " << endl;
    sem->v();
    cout << "Low done" << endl;

    return 0;
}

int func_m()
{
    cout << "Executing medium " << endl;
    cout << "Medium tries to get semaphore" << endl;
    sem->p();
    cout << "Medium gets semaphore " << endl;
    cout << "Create thread High" << endl;
    thread_h = new Periodic_Thread(RTConf(period_h * 1000, 0, 0, 0, iterations, wcet_h * 1000), &func_h);
    cout << "Executing medium inside semaphore" << endl;
    cout << "Medium releases semaphore " << endl;
    sem->v();
    cout << "Medium done " << endl;
    return 0;
}

int func_h()
{
    cout << "Executing high " << endl;
    cout << "High tries to get semaphore" << endl;
    cout << "Create thread Medium Low" << endl;
    thread_ml = new Periodic_Thread(RTConf(period_ml * 1000, 0, 0, 0, iterations, wcet_ml * 1000), &func_ml);
    sem->p();
    cout << "High gets semaphore " << endl;
    cout << "Executing high inside semaphore" << endl;
    cout << "High releases semaphore " << endl;
    sem->v();
    cout << "High done " << endl;
    return 0;
}

int func_ml()
{
    cout << "Executing Medium Low " << endl;
    cout << "Medium Low done " << endl;
    return 0;
}