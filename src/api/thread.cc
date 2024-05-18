// EPOS Thread Implementation

#include <machine.h>
#include <system.h>
#include <process.h>
#include <time.h>

extern "C" { volatile unsigned long _running() __attribute__ ((alias ("_ZN4EPOS1S6Thread4selfEv"))); }

__BEGIN_SYS

bool Thread::_not_booting;
volatile unsigned int Thread::_thread_count;
Scheduler_Timer * Thread::_timer;
Scheduler<Thread> Thread::_scheduler;
Spin Thread::_spin;

void Thread::constructor_prologue(unsigned int stack_size)
{
    lock();

    _thread_count++;
    if (!_not_booting)
        criterion().set_queue(CPU::id());
    else
        set_scheduler_queue();
   
    update_priorities(criterion().queue());
    _scheduler.insert(this);

    _stack = new (SYSTEM) char[stack_size];
}


void Thread::constructor_epilogue(Log_Addr entry, unsigned int stack_size)
{
    db<Thread>(TRC) << "Thread(entry=" << entry
                    << ",state=" << _state
                    << ",priority=" << _link.rank()
                    << ",stack={b=" << reinterpret_cast<void *>(_stack)
                    << ",s=" << stack_size
                    << "},context={b=" << _context
                    << "," << *_context << "}) => " << this << endl;

    assert((_state != WAITING) && (_state != FINISHING)); // invalid states

    if((_state != READY) && (_state != RUNNING))
        _scheduler.suspend(this);

    if(preemptive && (_state == READY) && (_link.rank() != IDLE)) {
        call_cpu_reschedule(criterion().queue());
    }

    unlock();
}


Thread::~Thread()
{
    lock();

    db<Thread>(TRC) << "~Thread(this=" << this
                    << ",state=" << _state
                    << ",priority=" << _link.rank()
                    << ",stack={b=" << reinterpret_cast<void *>(_stack)
                    << ",context={b=" << _context
                    << "," << *_context << "})" << endl;

    // The running thread cannot delete itself!
    assert(_state != RUNNING);

    switch(_state) {
    case RUNNING:  // For switch completion only: the running thread would have deleted itself! Stack wouldn't have been released!
        exit(-1);
        break;
    case READY:
        _scheduler.remove(this);
        _thread_count--;
        break;
    case SUSPENDED:
        _scheduler.resume(this);
        _scheduler.remove(this);
        _thread_count--;
        break;
    case WAITING:
        _waiting->remove(this);
        _scheduler.resume(this);
        _scheduler.remove(this);
        _thread_count--;
        break;
    case FINISHING: // Already called exit()
        break;
    }

    for (auto i = _synchronizer_running_queue.begin(); i != _synchronizer_running_queue.end();) {
        i->object()->remove(this);
        auto next = i->next();
        delete i;
        i = next;
    }
    for (auto i = _synchronizer_modified_queue.begin(); i != _synchronizer_modified_queue.end();) {
        i->object()->remove(this);
        auto next = i->next();
        delete i;
        i = next;
    }

    if(_joining)
        _joining->resume();

    unlock();

    delete _stack;
}


void Thread::set_scheduler_queue()
{
    assert(locked());
    unsigned int smaller_queue_index = 0;
    unsigned int smaller_queue_size = _scheduler.schedulables_at(0);
    for (unsigned int i = 1; i < Traits<Machine>::CPUS; i++) {
        auto size = _scheduler.schedulables_at(i);
        if (size < smaller_queue_size) {
            smaller_queue_index = i;
            smaller_queue_size = size;
        }
    }

    criterion().set_queue(smaller_queue_index);
}

void Thread::priority(const Criterion & c)
{
    lock();
    db<Thread>(TRC) << "Thread::priority(this=" << this << ",prio=" << c << ")" << endl;

    if(_state == READY) { // reorder the scheduling queue
        _scheduler.remove(this);
        _link.rank(c);
        _scheduler.insert(this);
    } else
        _link.rank(c);

    if(preemptive)
        call_cpu_reschedule(criterion().queue());

    unlock();
}


int Thread::join()
{
    lock();

    db<Thread>(TRC) << "Thread::join(this=" << this << ",state=" << _state << ")" << endl;
    // Precondition: no Thread::self()->join()
    assert(running() != this);

    // Precondition: a single joiner
    assert(!_joining);

    if(_state != FINISHING) {
        Thread * prev = running();
        prev->criterion().update_total_execution_time();

        _joining = prev;
        prev->_state = SUSPENDED;
        _scheduler.suspend(prev); // implicitly choose() if suspending chosen()

        Thread * next = _scheduler.chosen();

        dispatch(prev, next);
    }

    unlock();

    return *reinterpret_cast<int *>(_stack);
}


void Thread::pass()
{
    lock();

    db<Thread>(TRC) << "Thread::pass(this=" << this << ")" << endl;

    Thread * prev = running();
    prev->criterion().update_total_execution_time();
    _scheduler.remove(this);
    criterion().update_priority();
    criterion().set_queue(prev->criterion().queue());
    update_priorities(criterion().queue());
    _scheduler.insert(this);
    Thread * next = _scheduler.choose(this);

    if(next)
        dispatch(prev, next, false);
    else
        db<Thread>(WRN) << "Thread::pass => thread (" << this << ") not ready!" << endl;

    unlock();
}


void Thread::suspend()
{
    lock();

    db<Thread>(TRC) << "Thread::suspend(this=" << this << ")" << endl;

    Thread * prev = running();
    prev->criterion().update_total_execution_time();

    _state = SUSPENDED;
    _scheduler.suspend(this);

    Thread * next = _scheduler.chosen();

    dispatch(prev, next);

    unlock();
}


void Thread::resume()
{
    lock();

    db<Thread>(TRC) << "Thread::resume(this=" << this << ")" << endl;

    if(_state == SUSPENDED) {
        _state = READY;
        set_scheduler_queue();
        if(dynamic) {
            criterion().update_priority();
            update_priorities(criterion().queue());
        }
        _scheduler.resume(this);

        if(preemptive)
            call_cpu_reschedule(criterion().queue());
    } else
        db<Thread>(WRN) << "Resume called for unsuspended object!" << endl;

    unlock();
}


void Thread::yield()
{
    lock();

    db<Thread>(TRC) << "Thread::yield(running=" << running() << ")" << endl;

    Thread * prev = running();
    prev->criterion().update_total_execution_time();
    update_priorities();

    Thread * next = _scheduler.choose_another();

    dispatch(prev, next);

    unlock();
}


void Thread::exit(int status)
{
    lock();

    db<Thread>(TRC) << "Thread::exit(status=" << status << ") [running=" << running() << "]" << endl;

    Thread * prev = running();
    _scheduler.remove(prev);
    prev->_state = FINISHING;
    *reinterpret_cast<int *>(prev->_stack) = status;

    _thread_count--;

    if(prev->_joining) {
        prev->_joining->_state = READY;
        prev->_joining->set_scheduler_queue();
        _scheduler.resume(prev->_joining);
        prev->_joining = 0;
    }

    Thread * next = _scheduler.choose(); // at least idle will always be there
    dispatch(prev, next);

    unlock();
}


void Thread::sleep(Queue * q)
{
    db<Thread>(TRC) << "Thread::sleep(running=" << running() << ",q=" << q << ")" << endl;
    assert(locked()); // locking handled by caller

    Thread * prev = running();
    prev->criterion().update_total_execution_time();
    _scheduler.suspend(prev);
    prev->_state = WAITING;
    prev->_waiting = q;
    q->insert(&prev->_link);

    Thread * next = _scheduler.chosen();

    dispatch(prev, next);
}


void Thread::wakeup(Queue * q)
{
    db<Thread>(TRC) << "Thread::wakeup(running=" << running() << ",q=" << q << ")" << endl;

    assert(locked()); // locking handled by caller
    if(!q->empty()) {
        Thread * t = q->remove()->object();
        t->_state = READY;
        t->_waiting = 0;
        if(dynamic)
            t->criterion().update_priority();
        t->set_scheduler_queue();
        update_priorities(t->criterion().queue());
        _scheduler.resume(t);

        if(preemptive)
            call_cpu_reschedule(t->criterion().queue());
    }
}


void Thread::wakeup_all(Queue * q)
{
    db<Thread>(TRC) << "Thread::wakeup_all(running=" << running() << ",q=" << q << ")" << endl;

    assert(locked()); // locking handled by caller


    if(!q->empty()) {
        for (unsigned int i = 0; i < Traits<Machine>::CPUS; i++) {
            update_priorities(i);
        }
        while(!q->empty()) {
            Thread * t = q->remove()->object();
            t->_state = READY;
            t->_waiting = 0;

            if (dynamic)
                t->criterion().update_priority();

            t->set_scheduler_queue();
            _scheduler.resume(t);
        }

        // ANNOTATION: Demais threads acordadas serão escalonadas no Quantum, se necessário
        if(preemptive) {
            for (unsigned int i = 0; i < Traits<Machine>::CPUS; i++) {
                call_cpu_reschedule(i);
            }
        }
    }
}

void Thread::call_cpu_reschedule(unsigned int cpu)
{
    assert(locked());

    if (Traits<Machine>::CPUS == 1) return reschedule();

    // auto cpu_id = lower_priority_thread_at_cpu();
    if (cpu == CPU::id()) return reschedule();

    IC::ipi(cpu, IC::INT_RESCHEDULER);

}

void Thread::rescheduler(IC::Interrupt_Id i)
{
    lock();
    reschedule();
    unlock();
}

void Thread::reschedule()
{
    if(!Criterion::timed || Traits<Thread>::hysterically_debugged)
        db<Thread>(TRC) << "Thread::reschedule()" << endl;

    assert(locked()); // locking handled by caller

    Thread * prev = running();

    auto * head = _scheduler.head();
    if (head == nullptr) return;
    Thread * t_next = head->object();
    if (t_next == prev || t_next->priority() >= prev->priority()) return;

    // Atualiza prioridade da thread, e ordena fila de threads para inserção em ordem correta
    prev->criterion().update_total_execution_time();
    if(dynamic) {
        prev->criterion().update_priority();
        update_priorities();
    }

    Thread * next = _scheduler.choose(); // insere _chosen anterior na fila
    dispatch(prev, next);
}

void Thread::update_priorities(unsigned int i) 
{
    db<Thread>(TRC) << "Thread::update_priorities()" << endl;

    assert(locked());

    if (!dynamic) return;

    for (auto item = _scheduler.begin(i); item != _scheduler.end(); item++) {
        Thread * t = item->object();
        if (t->_link.rank() == IDLE || t->_link.rank() == MAIN) continue;
        t->criterion().update_priority();
    }
}

Thread * volatile Thread::self()
{
    return _not_booting ? running() : reinterpret_cast<Thread * volatile>(CPU::id() + 1);
}

void Thread::dispatch(Thread * prev, Thread * next, bool charge)
{
    // "next" is not in the scheduler's queue anymore. It's already "chosen"
    if(charge) {
        if(Criterion::timed)
            _timer->restart();
    }

    if(prev != next) {
        if(prev->_state == RUNNING)
            prev->_state = READY;
        next->_state = RUNNING;

        next->criterion().set_last_started_time(Alarm::elapsed());

        // db<Thread>(TRC) << "Thread::dispatch(prev=" << prev << ",next=" << next << ")" << endl;
        if(Traits<Thread>::debugged && Traits<Debug>::info) {
            CPU::Context tmp;
            tmp.save();
            // db<Thread>(INF) << "Thread::dispatch:prev={" << prev << ",ctx=" << tmp << "}" << endl;
        }
        // db<Thread>(INF) << "Thread::dispatch:next={" << next << ",ctx=" << *next->_context << "}" << endl;

        // The non-volatile pointer to volatile pointer to a non-volatile context is correct
        // and necessary because of context switches, but here, we are locked() and
        // passing the volatile to switch_constext forces it to push prev onto the stack,
        // disrupting the context (it doesn't make a difference for Intel, which already saves
        // parameters on the stack anyway).
        db<Thread>(INF) << "\nCPU::switch_context -> SP = " << CPU::sp() << " EPC = " << hex << CPU::epc() << endl;

        // ANNOTATION: Não usa unlock pois habilita as interrupções
        if (Traits<Machine>::CPUS > 1)
            _spin.release();

        CPU::switch_context(const_cast<Context **>(&prev->_context), next->_context);

        // ANNOTATION: Ao retornar a execução da thread, as interrupções podem ter sido ligadas na CPU
        // portanto é importante que as interrupções sejam desligadas para respeitar restrições do locked.
        lock();
    }
}


int Thread::idle()
{
    db<Thread>(TRC) << "Thread::idle(this=" << running() << ")" << endl;
    while(_thread_count > Traits<Machine>::CPUS) { // someone else besides idle
        if(Traits<Thread>::trace_idle)
            db<Thread>(TRC) << "Thread::idle(this=" << running() << ")" << endl;

        CPU::int_enable();
        CPU::halt();

        if(!preemptive)
            yield();
    }

    CPU::int_disable();
    if (CPU::id() == 0)
        db<Thread>(WRN) << "The last thread has exited!" << endl;
    if(reboot) {
        if (CPU::id() == 0)
            db<Thread>(WRN) << "Rebooting the machine ..." << endl;
        Machine::reboot();
    } else {
        if (CPU::id() == 0)
            db<Thread>(WRN) << "Halting the machine ..." << endl;
        CPU::halt();
    }    

    return 0;
}

unsigned int Thread::lower_priority_thread_at_cpu() {
    assert(locked());

    Thread * t = _scheduler.chosen_at(0);
    if (t == nullptr) return 0;

    unsigned int cpu_id = 0;
    for (unsigned int i = 1; i < Traits<Machine>::CPUS; i++) {
        Thread * temp_t = _scheduler.chosen_at(i);
        if (temp_t == nullptr) return i;
        if (t->priority() < temp_t->priority()) {
            t = temp_t;
            cpu_id = i;
        }
    }

    return cpu_id;
}

void Thread::set_borrowed_priority(int p) {
    criterion().set_borrowed_priority(p);
    if (state() == READY) {
        _scheduler.remove(this);
        _scheduler.insert(this);
    }
}

void Thread::remove_borrowed_priority() {
    criterion().set_original_priority();
    if (state() == READY) {
        _scheduler.remove(this);
        _scheduler.insert(this);
    }
}

void Thread::insert_synchronizer(Synchronizer_Common *s) {
    auto link = new Synchronizer_List_Element(s);
    _synchronizers.insert_head(link);
}

void Thread::remove_synchronizer(Synchronizer_Common *s) {
    auto link = _synchronizers.remove(s);
    delete link;
}

void Thread::insert_synchronizer_running_queue(List<Thread> *q) {
    auto link = new Synchronizer_Thread_List_Element(q);
    _synchronizer_running_queue.insert_head(link);
}

void Thread::remove_synchronizer_running_queue(List<Thread> *q) {
    auto link = _synchronizer_running_queue.remove(q);
    delete link;
}

void Thread::insert_synchronizer_modified_queue(List<Thread> *q) {
    auto link = new Synchronizer_Thread_List_Element(q);
    _synchronizer_modified_queue.insert_head(link);
}

void Thread::remove_synchronizer_modified_queue(List<Thread> *q) {
    auto link = _synchronizer_modified_queue.remove(q);
    delete link;
}

__END_SYS
