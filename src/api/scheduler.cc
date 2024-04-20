// EPOS CPU Scheduler Component Implementation

#include <process.h>
#include <time.h>

__BEGIN_SYS

Real_Time_Scheduler_Common::Real_Time_Scheduler_Common(int i, const Microsecond & d, const Microsecond & p, const Microsecond & c, const Microsecond & e):
    Priority(i), _deadline(d), _period(p), _capacity(c), _expected_execution_time(e) {
        _absolute_deadline = Alarm::elapsed() + _deadline;
    }

void Real_Time_Scheduler_Common::set_borrowed_priority(int p) {
    _using_borrowed_priority = true;
    _priority = p;
}

void Real_Time_Scheduler_Common::set_original_priority() {
    _using_borrowed_priority = false;
    update_priority();
}

// The following Scheduling Criteria depend on Alarm, which is not available at scheduler.h
template <typename ... Tn>
FCFS::FCFS(int p, Tn & ... an): Priority((p == IDLE) ? IDLE : Alarm::elapsed()) {}

EDF::EDF(const Microsecond & d, const Microsecond & p, const Microsecond & c, unsigned int): Real_Time_Scheduler_Common(Alarm::ticks(d), Alarm::ticks(d), p, c) {}

void EDF::update() {
    if((_priority >= PERIODIC) && (_priority < APERIODIC))
        _priority = Alarm::elapsed() + _deadline;
}

LLF::LLF(const Microsecond & d, const Microsecond & p, const Microsecond & c, unsigned int, const Microsecond & expected_execution_time):
    Real_Time_Scheduler_Common(Alarm::elapsed() + Alarm::ticks(d) - Alarm::ticks(expected_execution_time), Alarm::ticks(d), p, c, Alarm::ticks(expected_execution_time)) {}

void LLF::update() {
    _absolute_deadline = Alarm::elapsed() + _deadline;
    _total_execution_time = 0;
    _last_started_time = 0;
    _priority = _deadline - _expected_execution_time;
}

void LLF::update_priority() {
    if (_priority == IDLE) return;
    if (_using_borrowed_priority) return;
    _priority = _absolute_deadline - Alarm::elapsed() - (_expected_execution_time - _total_execution_time);
}

void LLF::update_total_execution_time() {
    _total_execution_time = _total_execution_time + Alarm::elapsed() - _last_started_time;
}

void LLF::set_original_priority() {
    _using_borrowed_priority = false;
    update_priority();
}

// Since the definition of FCFS above is only known to this unit, forcing its instantiation here so it gets emitted in scheduler.o for subsequent linking with other units is necessary.
template FCFS::FCFS<>(int p);

__END_SYS
