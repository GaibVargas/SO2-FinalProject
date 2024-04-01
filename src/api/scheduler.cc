// EPOS CPU Scheduler Component Implementation

#include <process.h>
#include <time.h>

__BEGIN_SYS

// The following Scheduling Criteria depend on Alarm, which is not available at scheduler.h
template <typename ... Tn>
FCFS::FCFS(int p, Tn & ... an): Priority((p == IDLE) ? IDLE : Alarm::elapsed()) {}

EDF::EDF(const Microsecond & d, const Microsecond & p, const Microsecond & c, unsigned int): Real_Time_Scheduler_Common(Alarm::ticks(d), Alarm::ticks(d), p, c) {}

void EDF::update() {
    if((_priority >= PERIODIC) && (_priority < APERIODIC))
        _priority = Alarm::elapsed() + _deadline;
}

LLF::LLF(const Microsecond & d, const Microsecond & p, const Microsecond & c, unsigned int, const Microsecond & expected_execution_time):
    Real_Time_Scheduler_Common(Alarm::elapsed() + Alarm::ticks(d) - Alarm::ticks(expected_execution_time), Alarm::ticks(d), p, c),
    _expected_execution_time(Alarm::ticks(expected_execution_time)),
    _absolute_deadline(Alarm::elapsed() + Alarm::ticks(d)) {}

void LLF::update() {
    // db<LLF>(ERR) << "\nExpected exectuion Time: " << _expected_execution_time << "ms\n";
    _absolute_deadline = Alarm::elapsed() + _deadline;
    _total_execution_time = 0;
    _last_started_time = 0;
    // db<LLF>(ERR) << "Deadline ABS: " << _absolute_deadline << "ms\n";
    if((_priority >= PERIODIC) && (_priority < APERIODIC))
        _priority = _absolute_deadline - Alarm::elapsed() - _expected_execution_time;
}

void LLF::update_priority() {
    if (_priority == IDLE) return;
    // db<LLF>(ERR) << "deadline " << _deadline << "tempo " << _total_execution_time << "\n";
    // db<LLF>(ERR) << "\nExpected execution Time: " << _expected_execution_time << "ms\n";
    if((_priority >= PERIODIC) && (_priority < APERIODIC)) {
        _priority = _absolute_deadline - Alarm::elapsed() - (_expected_execution_time - _total_execution_time);
    }
}

// Since the definition of FCFS above is only known to this unit, forcing its instantiation here so it gets emitted in scheduler.o for subsequent linking with other units is necessary.
template FCFS::FCFS<>(int p);

__END_SYS
