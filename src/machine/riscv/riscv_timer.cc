// EPOS RISC-V Timer Mediator Implementation

#include <machine/ic.h>
#include <machine/timer.h>

__BEGIN_SYS

Timer * Timer::_channels[CHANNELS];

void Timer::int_handler(Interrupt_Id i)
{
    // ANNOTATION: Apenas o core 0 conta o tempo do sistema(elapsed) e chama os alarmes necessários
    if (CPU::id() == 0) {
        if(_channels[ALARM] && (--_channels[ALARM]->_current[0] <= 0)) {
            _channels[ALARM]->_current[0] = _channels[ALARM]->_initial;
            _channels[ALARM]->_handler(i);
        }
    }

    if(_channels[SCHEDULER] && (--_channels[SCHEDULER]->_current[CPU::id()] <= 0)) {
        _channels[SCHEDULER]->_current[CPU::id()] = _channels[SCHEDULER]->_initial;
        _channels[SCHEDULER]->_handler(i);
    }
}

__END_SYS
