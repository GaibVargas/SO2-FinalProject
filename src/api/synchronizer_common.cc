// EPOS Synchronizer_Common Implementation

#include <synchronizer.h>
#include <utility/list.h>

__BEGIN_SYS

void Synchronizer_Common::sleep() {
    update_waiting_queue_priorities();
    auto t = Thread::running();
    if (!_queue.empty() && _queue.head()->object()->priority() < t->priority()) {
        t = _queue.head()->object();
    }
    pass_priority_to_threads(t);
    Thread::sleep(&_queue);
    acquire_synchronyzer(Thread::running());
}

void Synchronizer_Common::acquire_synchronyzer(Thread *t) {
    auto link_thread = new Thread_List_Element(t);
    _running_queue.insert(link_thread);

    t->insert_synchronizer(this);
    t->insert_synchronizer_running_queue(&_running_queue);
}

void Synchronizer_Common::release_synchronyzer(Thread *t) {
    t->remove_synchronizer_running_queue(&_running_queue);
    t->remove_synchronizer(this);

    auto link_running = _running_queue.remove(t);
    delete link_running;

    auto link_modified = _modified_threads.remove(t);
    if (!link_modified) return;
    delete link_modified;
    t->remove_synchronizer_modified_queue(&_modified_threads);
    set_next_priority(t);
}

void Synchronizer_Common::pass_priority_to_threads(Thread *t) {
    // ANNOTATION: a thread de prioridade mais alta dentro da região crítica, com prioridade mais baixa que t
    Thread *prioritize_thread = nullptr;
    for (auto i = _running_queue.begin(); i != _running_queue.end(); i++) {
        if (i->object()->priority() > t->priority()) {
            if (prioritize_thread == nullptr)
                prioritize_thread = i->object();
            else if (prioritize_thread->priority() > i->object()->priority())
                prioritize_thread = i->object();
        }
    }
    if (prioritize_thread) {
        prioritize_thread->set_borrowed_priority(t->priority());
        if (!_modified_threads.search(prioritize_thread)) {
            _modified_threads.insert(new Thread_List_Element(prioritize_thread));
            prioritize_thread->insert_synchronizer_modified_queue(&_modified_threads);
        }
    }
}

void Synchronizer_Common::remove_all_lent_priorities() {
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
}

void Synchronizer_Common::set_next_priority(Thread *t) {
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

__END_SYS