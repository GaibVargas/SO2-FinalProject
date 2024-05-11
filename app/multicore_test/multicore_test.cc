#include <time.h>
#include <real-time.h>

using namespace EPOS;

const unsigned int wcet = 200; // ms
const int threads_number = 10;

OStream cout;
Chronometer chrono;
Periodic_Thread * threads[threads_number];

int exec(int id);

int main()
{
    cout << "Multicore Test" << endl;

    cout << "\nThis test consists in creating " << 4 << " periodic threads that executes for " << wcet << "ms each" << endl;

    cout << "Threads will now be created and I'll wait for them to finish..." << endl;

    chrono.start();

    for (int i = 0; i < threads_number; i++) {
        threads[i] = new Periodic_Thread(RTConf((wcet * 2) * 1000, 0, 0, 0, 0, wcet * 1000), &exec, i);
    }
    for (int i = 0; i < threads_number; i++) {
        threads[i]->join();
    }

    chrono.stop();

    cout << "The measured time was " << chrono.read() / 1000 <<" ms!" << endl;
    cout << "Done!";

    for (int i = 0; i < threads_number; i++) {
        delete threads[i];
    }

    return 0;
}

int exec(int id) {
    Microsecond elapsed = chrono.read() / 1000;
    for(Microsecond end = elapsed + wcet, last = end; end > elapsed; elapsed = chrono.read() / 1000)
        if(last != elapsed) {
            cout << "<" << id << ">Executing " << id << "</" << id << ">" << endl;
            last = elapsed;
        }
    return 0;
}
