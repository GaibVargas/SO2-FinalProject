// EPOS Periodic Thread Component Test Program

#include <time.h>
#include <real-time.h>

using namespace EPOS;

const unsigned int iterations = 1;
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
Periodic_Thread * thread_l; // low
Periodic_Thread * thread_m; // medium
Periodic_Thread * thread_h; // high

Mutex * mutex;

int main()
{
    cout << "Testing PCP..." << endl;

    mutex = new Mutex();
    thread_l = new Periodic_Thread(RTConf(period_l * 1000, 0, 0, 0, iterations, wcet_l * 1000), &func_l);

    chrono.start();

    thread_l->join();
    cout << "DEu join na medium\n";
    thread_m->join();
    cout << "Liberou o join na medium\n";
    thread_h->join();

    chrono.stop();

    cout << "I'm also done, bye!" << endl;

    return 0;
}

int func_l()
{
    cout << "Executing low \n";
    mutex->lock();
    cout << "Low gets mutex \n";
    thread_h = new Periodic_Thread(RTConf(period_h * 1000, 0, 0, 0, iterations, wcet_h * 1000), &func_h);
    cout << "\t[p(L)=" << thread_l->priority()
    << ", p(M)=" << thread_m->priority()
    << ", p(H)=" << thread_h->priority() << "]";
    cout << "Executing low \n";
    mutex->unlock();
    cout << "Low done\n";

    return 0;
}

int func_m()
{
    // cout << "\t[p(L)=" << thread_l->priority()
    // << ", p(M)=" << thread_m->priority()
    // << ", p(H)=" << thread_h->priority() << "]";

    cout << "Executing medium \n";
    cout << "Medium done \n";
    return 0;
}

int func_h()
{
    cout << "Executing high \n";
    thread_m = new Periodic_Thread(RTConf(period_m * 1000, 0, 0, 0, iterations, wcet_m * 1000), &func_m);
    // cout << "\t[p(L)=" << thread_l->priority()
    // << ", p(M)=" << thread_m->priority()
    // << ", p(H)=" << thread_h->priority() << "]";
    cout << "Executing high \n";
    mutex->lock();
    cout << "High gets mutex \n";
    cout << "Executing high \n";
    mutex->unlock();
    cout << "High done \n";
    cout << "\t[p(L)=" << thread_l->priority()
    << ", p(M)=" << thread_m->priority()
    << ", p(H)=" << thread_h->priority() << "]";
    return 0;
}
