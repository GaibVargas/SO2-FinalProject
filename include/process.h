// EPOS Thread Component Declarations

#ifndef __process_h
#define __process_h

#include <architecture.h>
#include <machine.h>
#include <utility/queue.h>
#include <utility/handler.h>
#include <scheduler.h>

extern "C" { void __exit(); }

__BEGIN_SYS

class Synchronizer_Common;
class Thread
{
    friend class Init_End;              // context->load()
    friend class Init_System;           // for init() on CPU != 0
    friend class Scheduler<Thread>;     // for link()
    friend class Synchronizer_Common;   // for lock() and sleep()
    friend class Alarm;                 // for lock()
    friend class System;                // for init()
    friend class IC;                    // for link() for priority ceiling
    friend class Mutex;
    friend class Semaphore;

    typedef List<Synchronizer_Common> Synchronizer_List;
    typedef List<Synchronizer_Common>::Element Synchronizer_List_Element;

    typedef List<List<Thread>> Synchronizer_Thread_List;
    typedef List<List<Thread>>::Element Synchronizer_Thread_List_Element;

protected:
    static const bool preemptive = Traits<Thread>::Criterion::preemptive;
    static const bool dynamic = Traits<Thread>::Criterion::dynamic;
    static const bool reboot = Traits<System>::reboot;

    static const unsigned int QUANTUM = Traits<Thread>::QUANTUM;
    static const unsigned int STACK_SIZE = Traits<Application>::STACK_SIZE;

    typedef CPU::Log_Addr Log_Addr;
    typedef CPU::Context Context;

public:
    // Thread State
    enum State {
        RUNNING,
        READY,
        SUSPENDED,
        WAITING,
        FINISHING
    };

    // Thread Scheduling Criterion
    typedef Traits<Thread>::Criterion Criterion;
    enum {
        ISR     = Criterion::ISR,
        HIGH    = Criterion::HIGH,
        NORMAL  = Criterion::NORMAL,
        HIGHEST = Criterion::HIGHEST,
        LOW     = Criterion::LOW,
        MAIN    = Criterion::MAIN,
        IDLE    = Criterion::IDLE
    };

    // Thread Queue
    typedef Ordered_Queue<Thread, Criterion, Scheduler<Thread>::Element> Queue;

    // Thread Configuration
    struct Configuration {
        Configuration(const State & s = READY, const Criterion & c = NORMAL, unsigned int ss = STACK_SIZE)
        : state(s), criterion(c), stack_size(ss) {}

        State state;
        Criterion criterion;
        unsigned int stack_size;
    };


public:
    template<typename ... Tn>
    Thread(int (* entry)(Tn ...), Tn ... an);
    template<typename ... Tn>
    Thread(const Configuration & conf, int (* entry)(Tn ...), Tn ... an);
    ~Thread();

    const volatile State & state() const { return _state; }
    const volatile Criterion::Statistics & statistics() { return criterion().statistics(); }

    const volatile Criterion & priority() const { return _link.rank(); }
    void priority(const Criterion & p);

    int join();
    void pass();
    void suspend();
    void resume();

    static Thread * volatile self() __attribute__((used));
    static void yield();
    static void exit(int status = 0);

protected:
    void constructor_prologue(unsigned int stack_size);
    void constructor_epilogue(Log_Addr entry, unsigned int stack_size);

    void set_scheduler_queue();

    Criterion & criterion() { return const_cast<Criterion &>(_link.rank()); }
    Queue::Element * link() { return &_link; }

    static Thread * volatile running() { return _scheduler.chosen(); }

    static void lock(Spin * _lock = &_spin) {
        CPU::int_disable();
        if (Traits<Machine>::CPUS > 1)
            _lock->acquire();
    }
    static void unlock(Spin * _lock = &_spin) {
        if (Traits<Machine>::CPUS > 1)
            _lock->release();
        CPU::int_enable();
    }
    static bool locked() { 
        if (Traits<Machine>::CPUS > 1)
            return _spin.taken() && CPU::int_disabled();
        return CPU::int_disabled();
    }

    static void sleep(Queue * q);
    static void wakeup(Queue * q);
    static void wakeup_all(Queue * q);

    static void update_priorities(unsigned int i = CPU::id());
    static void reschedule();
    static void rescheduler(IC::Interrupt_Id i);
    static void call_cpu_reschedule(unsigned int cpu = CPU::id());

    static void dispatch(Thread * prev, Thread * next, bool charge = true);

    static int idle();

    void set_borrowed_priority(int p = HIGHEST);
    void remove_borrowed_priority();
    void insert_synchronizer(Synchronizer_Common *s);
    void remove_synchronizer(Synchronizer_Common *s);

    void insert_synchronizer_running_queue(List<Thread> *q);
    void remove_synchronizer_running_queue(List<Thread> *q);
    void insert_synchronizer_modified_queue(List<Thread> *q);
    void remove_synchronizer_modified_queue(List<Thread> *q);

private:
    static void init();

protected:
    char * _stack;
    Context * volatile _context;
    volatile State _state;
    Queue * _waiting;
    Thread * volatile _joining;
    Queue::Element _link;

    // Lista de sincronizadores usada na função Synchronizer_Common::set_next_priority(Thread *t).
    // Sua função é tratar o aninhamento de sincronizadores.
    Synchronizer_List _synchronizers;

    // Lista de listas de threads dos sincronizadores.
    // O objetivo dessa lista é permitir que a thread se auto remova da lista caso seja destruída, 
    // evitando vazamento de memória.
    Synchronizer_Thread_List _synchronizer_running_queue;
    Synchronizer_Thread_List _synchronizer_modified_queue;

    static Spin _spin;
    static bool _not_booting;
    static volatile unsigned int _thread_count;
    static Scheduler_Timer * _timer;
    static Scheduler<Thread> _scheduler;
};


template<typename ... Tn>
inline Thread::Thread(int (* entry)(Tn ...), Tn ... an)
: _state(READY), _waiting(0), _joining(0), _link(this, NORMAL)
{
    constructor_prologue(STACK_SIZE);
    _context = CPU::init_stack(0, _stack + STACK_SIZE, &__exit, entry, an ...);
    constructor_epilogue(entry, STACK_SIZE);
}

template<typename ... Tn>
inline Thread::Thread(const Configuration & conf, int (* entry)(Tn ...), Tn ... an)
: _state(conf.state), _waiting(0), _joining(0), _link(this, conf.criterion)
{
    constructor_prologue(conf.stack_size);
    _context = CPU::init_stack(0, _stack + conf.stack_size, &__exit, entry, an ...);
    constructor_epilogue(entry, conf.stack_size);
}


// A Java-like Active Object
class Active: public Thread
{
public:
    Active(): Thread(Configuration(Thread::SUSPENDED), &entry, this) {}
    virtual ~Active() {}

    virtual int run() = 0;

    void start() { resume(); }

private:
    static int entry(Active * runnable) { return runnable->run(); }
};


// An event handler that triggers a thread (see handler.h)
class Thread_Handler : public Handler
{
public:
    Thread_Handler(Thread * h) : _handler(h) {}
    ~Thread_Handler() {}

    void operator()() { _handler->resume(); }

private:
    Thread * _handler;
};

__END_SYS

#endif
