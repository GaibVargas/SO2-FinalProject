// EPOS System Initialization

#include <system.h>
#include <time.h>
#include <process.h>

__BEGIN_SYS

void System::init()
{
    if (CPU::id() == 0) {
        if(Traits<Alarm>::enabled)
            Alarm::init();
    }

    if(Traits<Thread>::enabled)
        Thread::init();
}

__END_SYS
