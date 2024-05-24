// EPOS Scheduler Component Declarations

#ifndef __scheduler_h
#define __scheduler_h

#include <architecture/cpu.h>
#include <architecture/pmu.h>
#include <architecture/tsc.h>
#include <utility/scheduling.h>
#include <utility/math.h>
#include <utility/convert.h>

__BEGIN_SYS

// All scheduling criteria, or disciplines, must define operator int() with
// the semantics of returning the desired order of a given object within the
// scheduling list
class Scheduling_Criterion_Common
{
    friend class _SYS::Thread;
    friend class _SYS::Periodic_Thread;
    friend class _SYS::RT_Thread;
    friend class _SYS::Clerk<System>;         // for _statistics

public:
    // Priorities
    enum : int {
        ISR    = -1000,
        MAIN   = -1,
        HIGHEST = -((1 << (sizeof(int) * 8 - 2)) - 1),
        HIGH   = 0,
        NORMAL = (unsigned(1) << (sizeof(int) * 8 - 2)) - 1,
        LOW    = (unsigned(1) << (sizeof(int) * 8 - 1)) - 2,
        IDLE   = (unsigned(1) << (sizeof(int) * 8 - 1)) - 1
    };

    // Constructor helpers
    enum : unsigned int {
        SAME        = 0,
        NOW         = 0,
        UNKNOWN     = 0,
        ANY         = -1U
    };

    // Policy types
    enum : int {
        PERIODIC    = HIGH,
        APERIODIC   = NORMAL,
        SPORADIC    = NORMAL
    };

    // Policy traits
    static const bool timed = false;
    static const bool dynamic = false;
    static const bool preemptive = true;
    static const bool collecting = false;
    static const bool charging = false;
    static const bool awarding = false;
    static const bool migrating = false;
    static const bool track_idle = false;
    static const bool task_wide = false;
    static const bool cpu_wide = false;
    static const bool system_wide = false;
    static const unsigned int QUEUES = 1;

    // Runtime Statistics (for policies that don't use any; that's why its a union)
    union Statistics {
        // Thread Execution Time
        TSC::Time_Stamp thread_execution_time;  // accumulated thread execution time
        TSC::Time_Stamp last_thread_dispatch;   // time stamp of last dispatch

        // Deadline Miss count - Used By Clerk
        Alarm * alarm_times;                    // pointer to RT_Thread private alarm (for monitoring purposes)
        unsigned int finished_jobs;             // number of finished jobs given by the number of times alarm->p() was called for this thread
        unsigned int missed_deadlines;          // number of missed deadlines given by the number of finished jobs (finished_jobs) minus the number of dispatched jobs (alarm_times->times)

        // CPU Execution Time (capture ts)
        static TSC::Time_Stamp _cpu_time[Traits<Build>::CPUS];              // accumulated CPU time in the current hyperperiod for each CPU
        static TSC::Time_Stamp _last_dispatch_time[Traits<Build>::CPUS];    // time Stamp of last dispatch in each CPU
        static TSC::Time_Stamp _last_activation_time;                       // global time stamp of the last heuristic activation
    };

protected:
    Scheduling_Criterion_Common() {}

public:
    const Microsecond period() { return 0;}
    void period(const Microsecond & p) {}

    unsigned int queue() const { return 0; }
    void queue(unsigned int q) {}
    void set_queue(unsigned int q) {}

    bool update() { return false; }

    bool collect(bool end = false) { return false; }
    bool charge(bool end = false) { return true; }
    bool award(bool end = false) { return true; }

    volatile Statistics & statistics() { return _statistics; }

    static void init() {}

protected:
    Statistics _statistics;
};

// Priority (static and dynamic)
class Priority: public Scheduling_Criterion_Common
{
    friend class _SYS::Thread;
    friend class _SYS::Periodic_Thread;
    friend class _SYS::RT_Thread;

public:
    template <typename ... Tn>
    Priority(int p = NORMAL, Tn & ... an): _priority(p) {}

    operator const volatile int() const volatile { return _priority; }
    // static unsigned int current_head() { return CPU::id(); }
    // static unsigned int current_queue() { return CPU::id(); }
    // const volatile unsigned int & queue() const volatile { return _queue; }
    // void set_queue(unsigned int i) { 
    //     if (i < 0 || i > Traits<Machine>::CPUS) {
    //         db<Priority>(WRN) << "A fila deve estar entre 0 e " << Traits<Machine>::CPUS - 1 << endl;
    //         Machine::panic();
    //     }
    //     _queue = i; 
    //     }

public:
    // static const unsigned int HEADS = Traits<Machine>::CPUS;
    // static const unsigned int QUEUES = Traits<Machine>::CPUS;

protected:
    volatile int _priority;
    // volatile unsigned int _queue;
};

// Round-Robin
class RR: public Priority
{
public:
    static const bool timed = true;
    static const bool dynamic = false;
    static const bool preemptive = true;

public:
    template <typename ... Tn>
    RR(int p = NORMAL, Tn & ... an): Priority(p) {}
};

// First-Come, First-Served (FIFO)
class FCFS: public Priority
{
public:
    static const bool timed = false;
    static const bool dynamic = false;
    static const bool preemptive = false;

public:
    template <typename ... Tn>
    FCFS(int p = NORMAL, Tn & ... an);
};


// Real-time Algorithms
class Real_Time_Scheduler_Common: public Priority
{
protected:
    Real_Time_Scheduler_Common(int p): Priority(p), _deadline(0), _period(0), _capacity(0), _expected_execution_time(0), _absolute_deadline(0) {} // aperiodic
    Real_Time_Scheduler_Common(int i, const Microsecond & d, const Microsecond & p, const Microsecond & c, const Microsecond & e = 0);

public:
    const Microsecond period() { return _period; }
    void period(const Microsecond & p) { _period = p; }

    void update_priority() {};

    Microsecond last_started_time() { return _last_started_time; }
    Microsecond total_execution_time() { return _total_execution_time; }

    void set_last_started_time(Microsecond time) { _last_started_time = time; }
    void update_total_execution_time() {};

    void set_borrowed_priority(int p = HIGHEST);
    void set_original_priority();

public:
    Microsecond _deadline;
    Microsecond _period;
    Microsecond _capacity;

    Microsecond _expected_execution_time;
    Microsecond _absolute_deadline;
    Microsecond _total_execution_time = 0;
    Microsecond _last_started_time = 0;

    bool _using_borrowed_priority = false;
};

// Rate Monotonic
class RM:public Real_Time_Scheduler_Common
{
public:
    static const bool timed = false;
    static const bool dynamic = false;
    static const bool preemptive = true;

public:
    RM(int p = APERIODIC): Real_Time_Scheduler_Common(p) {}
    RM(const Microsecond & d, const Microsecond & p = SAME, const Microsecond & c = UNKNOWN, unsigned int cpu = ANY)
    : Real_Time_Scheduler_Common(p ? p : d, d, p, c) {}
};

// Deadline Monotonic
class DM: public Real_Time_Scheduler_Common
{
public:
    static const bool timed = false;
    static const bool dynamic = false;
    static const bool preemptive = true;

public:
    DM(int p = APERIODIC): Real_Time_Scheduler_Common(p) {}
    DM(const Microsecond & d, const Microsecond & p = SAME, const Microsecond & c = UNKNOWN, unsigned int cpu = ANY)
    : Real_Time_Scheduler_Common(d, d, p, c) {}
};

// Earliest Deadline First
class EDF: public Real_Time_Scheduler_Common
{
public:
    static const bool timed = true;
    static const bool dynamic = true;
    static const bool preemptive = true;

public:
    EDF(int p = APERIODIC): Real_Time_Scheduler_Common(p) {}
    EDF(const Microsecond & d, const Microsecond & p = SAME, const Microsecond & c = UNKNOWN, unsigned int cpu = ANY);

    void update();
};

// Least Laxity First
class LLF: public Real_Time_Scheduler_Common
{
public:
    static const bool timed = true;
    static const bool dynamic = true;
    static const bool preemptive = true;

public:
    LLF(int p = APERIODIC): Real_Time_Scheduler_Common(p) {}
    LLF(const Microsecond & d, const Microsecond & p = SAME, const Microsecond & c = UNKNOWN, unsigned int cpu = ANY, const Microsecond & expected_execution_time = 0);

    void update();
    void update_priority();
    void set_original_priority();

    void update_total_execution_time();
};

class GLLF: public LLF
{
public:
    GLLF(int p = APERIODIC): LLF(p) {}
    GLLF(
        const Microsecond & d, const Microsecond & p = SAME, const Microsecond & c = UNKNOWN, unsigned int cpu = ANY, const Microsecond & expected_execution_time = 0
    ): LLF(d, p, c, cpu, expected_execution_time) {}

    static unsigned int current_head() { return CPU::id(); }

public:
    static const unsigned int HEADS = Traits<Machine>::CPUS;
};

class PLLF: public LLF
{
public:
    PLLF(int p = APERIODIC): LLF(p), _queue(CPU::id()) {}
    PLLF(
        const Microsecond & d, const Microsecond & p = SAME, const Microsecond & c = UNKNOWN, unsigned int cpu = ANY, const Microsecond & expected_execution_time = 0
    ): LLF(d, p, c, cpu, expected_execution_time) {
        if (cpu != ANY)
            _queue = cpu;
        else
            _queue = CPU::id();
    }

    static unsigned int current_queue() { return CPU::id(); }
    const volatile unsigned int & queue() const volatile { return _queue; }
    void set_queue(unsigned int i) { 
        if (i < 0 || i > Traits<Machine>::CPUS) {
            db<Priority>(WRN) << "A fila deve estar entre 0 e " << Traits<Machine>::CPUS - 1 << endl;
            Machine::panic();
        }
        _queue = i; 
    }

public:
    static const unsigned int QUEUES = Traits<Machine>::CPUS;

protected:
    volatile unsigned int _queue;
};

template<typename T>
class Scheduling_Queue<T, GLLF>:
public Multihead_Scheduling_List<T> {};

template<typename T>
class Scheduling_Queue<T, PLLF>:
public Scheduling_Multilist<T> {};

template<typename T, typename U>
struct is_same {
    static const bool value = false;
};

template<typename T>
struct is_same<T, T> {
    static const bool value = true;
};

__END_SYS


#endif
