// EPOS Thread Initialization

#include <machine/timer.h>
#include <machine/ic.h>
#include <system.h>
#include <process.h>

__BEGIN_SYS

extern "C" { void __epos_app_entry(); }

void Thread::init()
{
    db<Init, Thread>(TRC) << "Thread::init()" << endl;

    Criterion::init();

    if (CPU::id() == 0) {

        typedef int (Main)();

        // If EPOS is a library, then adjust the application entry point to __epos_app_entry, which will directly call main().
        // In this case, _init will have already been called, before Init_Application to construct MAIN's global objects.
        Main * main = reinterpret_cast<Main *>(__epos_app_entry);

        new (SYSTEM) Thread(Thread::Configuration(Thread::RUNNING, Thread::MAIN), main);
    } 

    // Idle thread creation does not cause rescheduling (see Thread::constructor_epilogue)
    new (SYSTEM) Thread(Thread::Configuration(Thread::READY, Thread::IDLE), &Thread::idle);

    // The installation of the scheduler timer handler does not need to be done after the
    // creation of threads, since the constructor won't call reschedule() which won't call
    // dispatch that could call timer->reset()
    // Letting reschedule() happen during thread creation is also harmless, since MAIN is
    // created first and dispatch won't replace it nor by itself neither by IDLE (which
    // has a lower priority)
    if (CPU::id() == 0) {
        if(Criterion::timed)
            _timer = new (SYSTEM) Scheduler_Timer(QUANTUM, rescheduler);
    }

    if (Traits<Machine>::CPUS > 1) {
        if (CPU::id() == 0)
            IC::int_vector(IC::INT_RESCHEDULER, Thread::rescheduler);
        IC::enable(IC::INT_RESCHEDULER);
    }

    // No more interrupts until we reach init_end
    CPU::int_disable();
    CPU::smp_barrier();

    // Transition from CPU-based locking to thread-based locking
    if (CPU::id() == 0) {
        _not_booting = true;
        Heap::_not_booting = true;
    }

    CPU::smp_barrier();
}

__END_SYS
