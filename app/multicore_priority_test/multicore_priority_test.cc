// EPOS Periodic Thread Component Test Program

#include <time.h>
#include <real-time.h>
#include <scheduler.h>

using namespace EPOS;

const unsigned int iterations = 0;
const unsigned int for_time = 30; // ms
const unsigned int period_l = 100000; // ms
const unsigned int period_m = 10000; // ms
const unsigned int period_h = 1000; // ms

const unsigned int wcet_l = 50; // ms
const unsigned int wcet_m = 50; // ms // ms
const unsigned int wcet_h = 50; // ms

int func();
int func_l();
int func_m();
int func_h();
long max(unsigned int a, unsigned int b, unsigned int c) { return ((a >= b) && (a >= c)) ? a : ((b >= a) && (b >= c) ? b : c); }

OStream cout;
Chronometer chrono;
Periodic_Thread * thread[3];
Periodic_Thread * thread_l; // low0
Periodic_Thread * thread_m; // low1
Periodic_Thread * thread_h; // low2

int main()
{
    cout << "Testando PI multicore..." << endl;

    thread_l = new Periodic_Thread(RTConf(period_l * 1000, 0, 0, 0, iterations, wcet_l * 1000), &func_l);
    thread_m = new Periodic_Thread(RTConf(period_m * 1000, 0, 0, 0, iterations, wcet_m * 1000), &func_m);
    thread_h = new Periodic_Thread(RTConf(period_h * 1000, 0, 0, 0, iterations, wcet_h * 1000), &func_h);
    for (unsigned int i = 0; i < 3; i++)
        thread[i] = new Periodic_Thread(RTConf((period_m + 100 )* 1000, 0, 0, 0, iterations, wcet_m * 1000), &func);
    chrono.start();

    thread_l->join();
    thread_m->join();
    thread_h->join();

    chrono.stop();

    cout << "I'm also done, bye!" << endl;

    delete thread_l;
    delete thread_m;
    delete thread_h;

    return 0;
}

int func_l()
{
    do {
        cout << "Executing low" << endl;

        Microsecond elapsed = chrono.read() / 1000;
        for(Microsecond end = elapsed + for_time + 500, last = end; end > elapsed; elapsed = chrono.read() / 1000) {
            if(last != elapsed) {
                if (elapsed%3 == 0)
                    cout << "Executing low p(l) = " << thread_l->priority() << endl;
                last = elapsed;
            }
        }
        
        cout << "Low done" << endl;
    } while (Periodic_Thread::wait_next());
    cout << "Low finish" << endl;
    return 0;
}

int func_m()
{
    do {
        cout << "Executing medium" << endl;

        Microsecond elapsed = chrono.read() / 1000;
        for(Microsecond end = elapsed + for_time + 500, last = end; end > elapsed; elapsed = chrono.read() / 1000) {
            if(last != elapsed) {
                if (elapsed%5 == 0)
                    cout << "Executing medium p(l) = " << thread_m->priority() << endl;
                last = elapsed;
            }
        }
       
    } while (Periodic_Thread::wait_next());
    cout << "Medium finish" << endl;
    return 0;
}

int func_h()
{
    do {
        cout << "Executing High" << endl;

        auto cl = LLF(1000000, 1000000, 0, 0, 50000);
        cl.set_queue(2);
        thread_l->priority(cl);

        Machine::delay(50000);

        auto cm = LLF(1000000, 1000000, 0, 0, 300000);
        cm.set_queue(2);
        thread_m->priority(cm);

        Microsecond elapsed = chrono.read() / 1000;
        for(Microsecond end = elapsed + for_time, last = end; end > elapsed; elapsed = chrono.read() / 1000) {
            if(last != elapsed) {
                if (elapsed%7 == 0)
                    cout << "Executing high p(l) = " << thread_h->priority() << endl;
                last = elapsed;
            }
        }


    } while (Periodic_Thread::wait_next());
    cout << "High finish" << endl;
    return 0;
}

int func()
{
    do {
        Microsecond elapsed = chrono.read() / 1000;
        for(Microsecond end = elapsed + for_time, last = end; end > elapsed; elapsed = chrono.read() / 1000)
            if(last != elapsed) {
                last = elapsed;
            }
       
    } while (Periodic_Thread::wait_next());
    return 0;
}
