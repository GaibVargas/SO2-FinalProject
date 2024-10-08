// EPOS Periodic Thread Component Test Program

#include <time.h>
#include <real-time.h>

using namespace EPOS;

const unsigned int iterations = 4;
const unsigned int period_a = 60; // ms
const unsigned int period_b = 65; // ms
const unsigned int period_c = 100; // ms

const unsigned int wcet_a = 10; // ms
const unsigned int wcet_b = 30; // ms
const unsigned int wcet_c = 40; // ms

int func_a();
int func_b();
int func_c();
long max(unsigned int a, unsigned int b, unsigned int c) { return ((a >= b) && (a >= c)) ? a : ((b >= a) && (b >= c) ? b : c); }

OStream cout;
Chronometer chrono;
Periodic_Thread * thread_a;
Periodic_Thread * thread_b;
Periodic_Thread * thread_c;

inline void exec(char c, unsigned int time = 0) // in miliseconds
{
    // Delay was not used here to prevent scheduling interference due to blocking
    Microsecond elapsed = chrono.read() / 1000;
    Microsecond counter = 0;
    

    cout << "\n" << elapsed << "\t" << c
         << "\t[p(A)=" << thread_a->priority()
         << ", p(B)=" << thread_b->priority()
         << ", p(C)=" << thread_c->priority() << "]";

    if(time) {
        for(Microsecond end = time, last = elapsed; end > counter; elapsed = chrono.read() / 1000)
            if(last != elapsed) {
                cout << "\n" << elapsed << "\t" << c
                    << "\t[p(A)=" << thread_a->priority()
                    << ", p(B)=" << thread_b->priority()
                    << ", p(C)=" << thread_c->priority() << "]";
                last = elapsed;
                // ANNOTATION: ainda não faz uso de todo o tempo que é dado
                // somar de um em um é o mesmo que assumir que o for é executado time vezes
                counter = counter + 1;
            }
    }

    cout << "\n" << c << " Terminei execução" << endl;
}


int main()
{
    cout << "Periodic Thread Component Test" << endl;

    cout << "\nThis test consists in creating three periodic threads as follows:" << endl;
    cout << "- Every " << period_a << "ms, thread A execs \"a\" waits for " << wcet_a << "ms \"a\";" << endl;
    cout << "- Every " << period_b << "ms, thread B execs \"b\" waits for " << wcet_b << "ms \"b\";" << endl;
    cout << "- Every " << period_c << "ms, thread C execs \"c\" waits for " << wcet_c << "ms \"c\";" << endl;

    cout << "Threads will now be created and I'll wait for them to finish..." << endl;

    // p,d,c,act,t
    thread_a = new Periodic_Thread(RTConf(period_a * 1000, 0, 0, 0, iterations, wcet_a * 1000), &func_a);
    thread_b = new Periodic_Thread(RTConf(period_b * 1000, 0, 0, 0, iterations, wcet_b * 1000), &func_b);
    thread_c = new Periodic_Thread(RTConf(period_c * 1000, 0, 0, 0, iterations, wcet_c * 1000), &func_c);

    exec('M');

    chrono.start();

    int status_a = thread_a->join();
    int status_b = thread_b->join();
    int status_c = thread_c->join();

    chrono.stop();

    exec('M');

    cout << "\n... done!" << endl;
    cout << "\n\nThread A exited with status \"" << char(status_a)
         << "\", thread B exited with status \"" << char(status_b)
         << "\" and thread C exited with status \"" << char(status_c) << "." << endl;

    cout << "\nThe estimated time to run the test was "
         << max(period_a, period_b, period_c) * iterations
         << " ms. The measured time was " << chrono.read() / 1000 <<" ms!" << endl;

    cout << "I'm also done, bye!" << endl;

    delete thread_a;
    delete thread_b;
    delete thread_c;

    return 0;
}

int func_a()
{
    exec('A');

    do {
        exec('a', wcet_a);
    } while (Periodic_Thread::wait_next());

    exec('A');

    return 'A';
}

int func_b()
{
    exec('B');

    do {
        exec('b', wcet_b);
    } while (Periodic_Thread::wait_next());

    exec('B');

    return 'B';
}

int func_c()
{
    exec('C');

    do {
        exec('c', wcet_c);
    } while (Periodic_Thread::wait_next());

    exec('C');

    return 'C';
}
