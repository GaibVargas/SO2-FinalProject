// EPOS Synchronizer_Common Implementation

#include <synchronizer.h>
#include <utility/list.h>

__BEGIN_SYS

void Synchronizer_Common::sleep() {
    assert(Thread::locked());

    update_waiting_queue_priorities();
    auto t = Thread::running();
    // Verifica a thread de maior prioridade no fila de Waiting do sincronizador
    if (_queue.empty() || _queue.head()->object()->priority() > t->priority()) {
        pass_priority_to_threads(t);
    }

    Thread::sleep(&_queue);
    acquire_synchronyzer(Thread::running());
}

void Synchronizer_Common::acquire_synchronyzer(Thread *t) {
    assert(Thread::locked());
    if (Traits<Thread>::priority_inversion_protocol == Traits<Build>::NOT) return;

    auto link_thread = new Thread_List_Element(t);
    _running_queue.insert(link_thread);

    t->insert_synchronizer(this);
    t->insert_synchronizer_running_queue(&_running_queue);
}

void Synchronizer_Common::release_synchronyzer(Thread *t) {
    assert(Thread::locked());
    if (Traits<Thread>::priority_inversion_protocol == Traits<Build>::NOT) return;

    t->remove_synchronizer_running_queue(&_running_queue);
    t->remove_synchronizer(this);

    auto link_running = _running_queue.remove(t);
    delete link_running;

    set_all_next_priority(t);
}

void Synchronizer_Common::pass_priority_to_threads(Thread *t) {
    assert(Thread::locked());
    if (Traits<Thread>::priority_inversion_protocol == Traits<Build>::NOT) return;

    for (auto i = 0U; i < Traits<Machine>::CPUS; i++)
        Thread::update_priorities(i);

    // ANNOTATION: sobe a prioridade de todas as threads de prioridade mais baixa dentro do synchronyzer
    for (auto i = _running_queue.begin(); i != _running_queue.end(); i++) {
        Thread *prioritize_thread = i->object();
        if (prioritize_thread->priority() > t->priority()) {
            prioritize_thread->set_borrowed_priority(t->priority());
            if (!_modified_threads.search(prioritize_thread)) {
                _modified_threads.insert(new Thread_List_Element(prioritize_thread));
                prioritize_thread->insert_synchronizer_modified_queue(&_modified_threads);
            }

            if (prioritize_thread->_state == Thread::READY) {
                Thread::_scheduler.remove(prioritize_thread);
                Thread::_scheduler.insert(prioritize_thread);
            }
        }
    }

    for (auto i = 0U; i < Traits<Machine>::CPUS; i++) {
        if (CPU::id() != i)
            IC::ipi(i, IC::INT_RESCHEDULER);
    }
}

void Synchronizer_Common::remove_all_lent_priorities() {
    assert(Thread::locked());
    if (Traits<Thread>::priority_inversion_protocol == Traits<Build>::NOT) return;

    for (auto i = _running_queue.begin(); i != _running_queue.end(); i++) {
        auto t = i->object(); 
        t->remove_synchronizer(this);
        t->remove_synchronizer_running_queue(&_running_queue);

        if (_modified_threads.search(t)) {
            t->remove_borrowed_priority();
            t->remove_synchronizer_modified_queue(&_modified_threads);
            set_next_priority(t);
        }
    }
    for (auto i = _running_queue.begin(); i != _running_queue.end();) {
        auto next = i->next();
        delete i;
        i = next;
    }
    for (auto i = _modified_threads.begin(); i != _modified_threads.end();) {
        auto next = i->next();
        delete i;
        i = next;
    }

    for (auto i = 0U; i < Traits<Machine>::CPUS; i++) {
        if (CPU::id() != i)
            IC::ipi(i, IC::INT_RESCHEDULER);
    }
}

void Synchronizer_Common::set_all_next_priority(Thread *thread_released)
{
    assert(Thread::locked());

    if (_modified_threads.size() < 1) return;

    for (auto i = 0U; i < Traits<Machine>::CPUS; i++)
        Thread::update_priorities(i);

    // Atualiza a prioridade de todas as threads modificadas por esse sincronizador
    for (auto i = _modified_threads.begin(); i != _modified_threads.end(); i++) {
        auto t = i->object();
        t->criterion().set_original_priority();

        int highest_priority = t->priority();
        Synchronizer_Common * from_sync = nullptr;

        // Procura nos sincronizadores em que a thread entrou, qual a próxima prioridade a ser herdada.
        for (auto s = t->_synchronizers.begin(); s != t->_synchronizers.end(); s++) {
            Synchronizer_Common *sync = s->object();

            // Marca que a thread que esta saindo não é mais modificada neste sincronizador
            if (sync == this && thread_released == t) {
                t->remove_synchronizer_modified_queue(&_modified_threads);
                continue;  
            } 

            sync->update_waiting_queue_priorities();

            Thread * t_sync;
            if (sync == this) // Desconsidera a cabeça que será acoradada pelo sincronizador
                t_sync = get_next_head_waiting();
            else 
                t_sync = sync->get_head_waiting();

            if (t_sync == nullptr) continue;

            if (t_sync->priority() < highest_priority) {
                highest_priority = t_sync->priority();
                from_sync = sync;
            }
        }

        if (highest_priority == t->priority()) continue;
        t->criterion().set_borrowed_priority(highest_priority);

        // Marca que a thread não é mais modificada neste sincronizador
        if (from_sync != this) {
            t->remove_synchronizer_modified_queue(&_modified_threads);
        }

        if (!from_sync->_modified_threads.search(t)) {
            from_sync->_modified_threads.insert(new Thread_List_Element(t));
            t->insert_synchronizer_modified_queue(&from_sync->_modified_threads);
        }

        if (t->state() == Thread::READY) {
            t->_scheduler.remove(t);
            t->_scheduler.insert(t);
        }
    }

    // Retira as threads que eram modificadas pelo sincronizador
    for (auto i = _modified_threads.begin(); i != _modified_threads.end();) {
        auto t = i->object();
        if (!t->_synchronizer_modified_queue.search(&_modified_threads)) {
            auto next = i->next();
            _modified_threads.remove(t);
            delete i;
            i = next;
        } else
            i = i->next();
    }

    for (auto i = 0U; i < Traits<Machine>::CPUS; i++) {
        if (CPU::id() != i)
            IC::ipi(i, IC::INT_RESCHEDULER);
    }
}

// Seleciona a próxima prioridade da thread no momento em que o sincronizador deixa de existir.
void Synchronizer_Common::set_next_priority(Thread *t) {
    assert(Thread::locked());

    t->criterion().set_original_priority();

    int highest_priority = t->priority();
    Synchronizer_Common * from_sync = nullptr;

    for (auto s = t->_synchronizers.begin(); s != t->_synchronizers.end(); s++) {
        Synchronizer_Common *sync = s->object();
        sync->update_waiting_queue_priorities();
        Thread * t_sync = sync->get_head_waiting();
        if (t_sync == nullptr) continue;

        if (t_sync->priority() < highest_priority) {
            highest_priority = t_sync->priority();
            from_sync = sync;
        }
    }

    if (highest_priority == t->priority()) return;
    t->criterion().set_borrowed_priority(highest_priority);
    if (!from_sync->_modified_threads.search(t)) {
        from_sync->_modified_threads.insert(new Thread_List_Element(t));
        t->insert_synchronizer_modified_queue(&from_sync->_modified_threads);
    }

    if (t->state() == Thread::READY) {
        t->_scheduler.remove(t);
        t->update_priorities(t->criterion().queue());
        t->_scheduler.insert(t);
    }
    
}

void Synchronizer_Common::update_waiting_queue_priorities() {
    for (auto i = _queue.begin(); i != _queue.end(); i++) {
        i->object()->criterion().update_priority();
    }
}

Thread* Synchronizer_Common::get_head_waiting() {
    auto element = _queue.head();
    if (!element) return nullptr;
    return element->object();
}

Thread* Synchronizer_Common::get_next_head_waiting() {
    auto element = _queue.head();
    if (!element || !element->next()) return nullptr;
    return element->next()->object();
}

__END_SYS