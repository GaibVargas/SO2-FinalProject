// EPOS Synchronizer Components

#ifndef __synchronizer_h
#define __synchronizer_h

#include <architecture.h>
#include <utility/handler.h>
#include <process.h>
#include <utility/list.h>

__BEGIN_SYS

class Synchronizer_Common
{
    friend class Thread;
protected:
    typedef Thread::Queue Queue;
    typedef List<Thread> Thread_List;
    typedef List<Thread>::Element Thread_List_Element;
    typedef Traits<Thread>::Criterion Criterion;

protected:
    Synchronizer_Common() {}
    ~Synchronizer_Common() {
        begin_atomic();
        remove_all_lent_priorities();
        wakeup_all();
        end_atomic(); 
    }

    // Atomic operations
    bool tsl(volatile bool & lock) { return CPU::tsl(lock); }
    long finc(volatile long & number) { return CPU::finc(number); }
    long fdec(volatile long & number) { return CPU::fdec(number); }

    // Thread operations
    void begin_atomic() { Thread::lock(); }
    void end_atomic() { Thread::unlock(); }

    void sleep();
    void wakeup() {
        release_synchronyzer(Thread::running());
        Thread::wakeup(&_queue); 
        Thread::call_cpu_reschedule();
    }
    void wakeup_all() { Thread::wakeup_all(&_queue); }

    void acquire_synchronyzer(Thread * t);
    void release_synchronyzer(Thread * t);
    void set_next_priority(Thread *t);
    void set_all_next_priority(Thread *thread_released);
    void pass_priority_to_threads(Thread * t);
    void remove_all_lent_priorities();
    void update_waiting_queue_priorities();
    Thread* get_head_waiting();
    Thread* get_next_head_waiting();

protected:
    Queue _queue;
    Thread_List _running_queue;
    Thread_List _modified_threads;
};


class Mutex: protected Synchronizer_Common
{
public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();

private:
    volatile bool _locked;
};


class Semaphore: protected Synchronizer_Common
{
public:
    Semaphore(long v = 1);
    ~Semaphore();

    void p();
    void v();

private:
    volatile long _value;
};


// This is actually no Condition Variable
// check http://www.cs.duke.edu/courses/spring01/cps110/slides/sem/sld002.htm
class Condition: protected Synchronizer_Common
{
public:
    Condition();
    ~Condition();

    void wait();
    void signal();
    void broadcast();
};


// An event handler that triggers a mutex (see handler.h)
class Mutex_Handler: public Handler
{
public:
    Mutex_Handler(Mutex * h) : _handler(h) {}
    ~Mutex_Handler() {}

    void operator()() { _handler->unlock(); }

private:
    Mutex * _handler;
};

// An event handler that triggers a semaphore (see handler.h)
class Semaphore_Handler: public Handler
{
public:
    Semaphore_Handler(Semaphore * h) : _handler(h) {}
    ~Semaphore_Handler() {}

    void operator()() { _handler->v(); }

private:
    Semaphore * _handler;
};

// An event handler that triggers a condition variable (see handler.h)
class Condition_Handler: public Handler
{
public:
    Condition_Handler(Condition * h) : _handler(h) {}
    ~Condition_Handler() {}

    void operator()() { _handler->signal(); }

private:
    Condition * _handler;
};

__END_SYS

#endif
