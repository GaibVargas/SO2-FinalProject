// EPOS Periodic Thread Component Test Program

#include <time.h>
#include <real-time.h>

using namespace EPOS;

const unsigned int iterations = 0;
const unsigned int for_time = 50; // ms
const unsigned int period_l = 100000; // ms
const unsigned int period_m = 10000; // ms
const unsigned int period_h = 1000; // ms

const unsigned int wcet_l = 50; // ms
const unsigned int wcet_m = 50; // ms
const unsigned int wcet_h = 50; // ms

const int n_threads_by_type = 3;

int func_l(int);
int func_m(int);
int func_h(int);

OStream cout;
Chronometer chrono;
Periodic_Thread * thread_l[n_threads_by_type]; // low
Periodic_Thread * thread_m[n_threads_by_type]; // medium
Periodic_Thread * thread_h[n_threads_by_type]; // high

int main()
{
    cout << "Testando escalonamento LLF multicore..." << endl;
    cout << "Usando GLLF espera-se que todas threads high executem primeiro, e assim por diante seguindo a prioridade das threads" << endl;
    cout << "Usando PLLF espera-se que as threads executem misturadas, pois cada fila conterá somente threads de um mesmo tipo" << endl;
    
    for (auto i = 0; i < n_threads_by_type; i++) {
        thread_h[i] = new Periodic_Thread(RTConf(period_h * 1000, 0, 0, 0, iterations, wcet_h * 1000), &func_h, i);
        thread_m[i] = new Periodic_Thread(RTConf(period_m * 1000, 0, 0, 0, iterations, wcet_m * 1000), &func_m, i);
        thread_l[i] = new Periodic_Thread(RTConf(period_l * 1000, 0, 0, 0, iterations, wcet_l * 1000), &func_l, i);
    }

    chrono.start();

    for (auto i = 0; i < n_threads_by_type; i++) {
        thread_h[i]->join();
        thread_m[i]->join();
        thread_l[i]->join();
    }

    chrono.stop();

    cout << endl << "I'm also done, bye!" << endl;

    
    for (auto i = 0; i < n_threads_by_type; i++) {
        delete thread_h[i];
        delete thread_m[i];
        delete thread_l[i];
    }

    return 0;
}

int func_l(int i)
{
    Machine::delay(5000);
    do {
        cout << endl;
        Microsecond elapsed = chrono.read() / 1000;
        for(Microsecond end = elapsed + for_time, last = end; end > elapsed; elapsed = chrono.read() / 1000) {
            if(last != elapsed) {
                // if (elapsed%3 == 0)
                    cout << "L" << i;
                last = elapsed;
            }
        }
    } while (Periodic_Thread::wait_next());
    return 0;
}

int func_m(int i)
{
    Machine::delay(5000);
    do {
        cout << endl;
        Microsecond elapsed = chrono.read() / 1000;
        for(Microsecond end = elapsed + for_time, last = end; end > elapsed; elapsed = chrono.read() / 1000) {
            if(last != elapsed) {
                // if (elapsed%5 == 0)
                    cout << "M" << i;
                last = elapsed;
            }
        }
    } while (Periodic_Thread::wait_next());
    return 0;
}

int func_h(int i)
{
    Machine::delay(5000);
    do {
        Microsecond elapsed = chrono.read() / 1000;
        for(Microsecond end = elapsed + for_time, last = end; end > elapsed; elapsed = chrono.read() / 1000) {
            if(last != elapsed) {
                // if (elapsed%7 == 0)
                    cout << "H" << i;
                last = elapsed;
            }
        }
    } while (Periodic_Thread::wait_next());
    return 0;
}
