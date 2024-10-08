// EPOS Semaphore Implementation

#include <synchronizer.h>

__BEGIN_SYS

Semaphore::Semaphore(long v) : _value(v)
{
    db<Synchronizer>(TRC) << "Semaphore(value=" << _value << ") => " << this << endl;
}


Semaphore::~Semaphore()
{
    db<Synchronizer>(TRC) << "~Semaphore(this=" << this << ")" << endl;
}


void Semaphore::p()
{
    db<Synchronizer>(TRC) << "Semaphore::p(this=" << this << ",value=" << _value << ")" << endl;

    begin_atomic();
    if(fdec(_value) < 1)
        sleep();
    else
        acquire_synchronyzer(Thread::running());
    end_atomic();
}


void Semaphore::v()
{
    db<Synchronizer>(TRC) << "Semaphore::v(this=" << this << ",value=" << _value << ")" << endl;

    begin_atomic();
    if(finc(_value) < 0)
        wakeup();
    else {
        release_synchronyzer(Thread::running());
        Thread::call_cpu_reschedule();
    }
    end_atomic();
}

__END_SYS
