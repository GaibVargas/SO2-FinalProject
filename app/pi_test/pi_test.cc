// EPOS Periodic Thread Component Test Program

#include <time.h>
#include <real-time.h>

using namespace EPOS;

const unsigned int iterations = 3;
const unsigned int period_l = 200; // ms
const unsigned int period_m = 150; // ms
const unsigned int period_h = 100; // ms

const unsigned int wcet_l = 10; // ms
const unsigned int wcet_m = 20; // ms
const unsigned int wcet_h = 30; // ms

int func_l();
int func_m();
int func_h();
long max(unsigned int a, unsigned int b, unsigned int c) { return ((a >= b) && (a >= c)) ? a : ((b >= a) && (b >= c) ? b : c); }

OStream cout;
Chronometer chrono;
Periodic_Thread *thread_l; // low
Periodic_Thread *thread_m; // medium
Periodic_Thread *thread_h; // high

Mutex *mutex1;
Mutex *mutex2;

int main()
{
    cout << "Testing PI..." << endl;

    mutex1 = new Mutex();
    mutex2 = new Mutex();
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
    do{
        cout << "Executing low p(l) = " << thread_l->priority() << endl;
        cout << "Low tries to get mutex 1" << endl;
        mutex1->lock();
        cout << "Low gets mutex 1" << endl;
        cout << "Executing low inside mutex 1 p(l) = " << thread_l->priority() << endl;
        cout << "Low tries to get mutex 2" << endl;
        mutex2->lock();
        cout << "Low gets mutex 2" << endl;
        if (!thread_h)
        {
            cout << "Low creates thread High " << endl;
            thread_h = new Periodic_Thread(RTConf(period_h * 1000, 0, 0, 0, iterations, wcet_h * 1000), &func_h);
        }
        cout << "Executing low inside mutex 2 p(l) = " << thread_l->priority() << endl;
        if (!thread_m)
        {
            cout << "Low creates thread Medium" << endl;
            thread_m = new Periodic_Thread(RTConf(period_m * 1000, 0, 0, 0, iterations, wcet_m * 1000), &func_m);
        }
        cout << "Executing low inside mutex 2 p(l) = " << thread_l->priority() << endl;
        cout << "Low releases mutex 2" << endl;
        mutex2->unlock();
        cout << "Executing low inside mutex 1 p(l) = " << thread_l->priority() << endl;
        cout << "Low releases mutex 1" << endl;
        mutex1->unlock();
        cout << "Executing low p(l) = " << thread_l->priority() << endl;
        cout << "Low job done" << endl;
    } while (Periodic_Thread::wait_next());

    cout << "Low thread done" << endl;

    return 0;
}

int func_m()
{
    do {
        cout << "Executing medium" << endl;
        cout << "Medium tries to get mutex 1" << endl;
        mutex1->lock();
        cout << "Medium gets mutex 1" << endl;
        cout << "Executing medium inside mutex 1" << endl;
        cout << "Medium releases mutex 1" << endl;
        mutex1->unlock();
        cout << "Medium job done" << endl;
    } while (Periodic_Thread::wait_next());

    cout << "Medium thread done" << endl;

    return 0;
}

int func_h()
{
    do {
        cout << "Executing high" << endl;
        cout << "High tries to get mutex 2" << endl;
        mutex2->lock();
        cout << "High gets mutex 2" << endl;
        cout << "Executing high inside mutex 2" << endl;
        cout << "High releases mutex 2" << endl;
        mutex2->unlock();
        cout << "High job done" << endl;
    } while (Periodic_Thread::wait_next());

    cout << "High thread done" << endl;

    return 0;
}
