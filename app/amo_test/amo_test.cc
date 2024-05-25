#include <architecture/rv64/rv64_cpu.h>

using namespace EPOS;

volatile bool lock = false;
int main()
{
    OStream cout;
    cout << "AMO test begin" << endl;

    {
        if(CPU::tsl(lock))
            cout << "tsl(): doesn't function properly!" << endl;
        else
            if(CPU::tsl(lock))
            	cout << "tsl(): ok" << endl;
            else
            	cout << "tsl(): doesn't function properly!" << endl;
    }
    {
        volatile int number = 100;
        volatile int tmp;
        if((tmp = CPU::finc(number)) != 100)
            cout << "finc(): doesn't function properly (n=" << tmp << ", should be 100)!" << endl;
        else
            if((tmp = CPU::finc(number)) != 101)
        	    cout << "finc(): doesn't function properly (n=" << tmp << ", should be 101)!" << endl;
            else
        	    cout << "finc(): ok" << endl;
    }
    {
        volatile int number = 100;
        volatile int tmp;
        if((tmp = CPU::fdec(number)) != 100)
            cout << "fdec(): doesn't function properly (n=" << tmp << ", should be 100)!" << endl;
        else
            if((tmp = CPU::fdec(number)) != 99)
        	    cout << "fdec(): doesn't function properly (n=" << tmp << ", should be 99)!" << endl;
            else
        	    cout << "fdec(): ok" << endl;
    }
    {
        volatile int number = 100;
        volatile int compare = number;
        volatile int replacement = number - 1;
        volatile int tmp;
        if((tmp = CPU::cas(number, compare, replacement)) != compare)
            cout << "cas(): doesn't function properly [1] (n=" << tmp << ", should be " << compare << ")!" << endl;
        else
            if((tmp = CPU::cas(number, compare, replacement)) != replacement)
                cout << "cas(): doesn't function properly [2] (n=" << tmp << ", should be " << replacement << ")!" << endl;
            else
                cout << "cas(): ok" << endl;
    }
 
    cout << "AMO test finished" << endl;

    return 0;
}
