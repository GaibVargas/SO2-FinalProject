// EPOS Synchronizer_Common Implementation

#include <synchronizer.h>
#include <utility/list.h>

__BEGIN_SYS

void Synchronizer_Common::insert_thread(Thread *t) {
    auto link = new Thread_List_Element(t);
    _running_queue.insert(link);
}

void Synchronizer_Common::remove_thread(Thread *t) {
    auto link = _running_queue.remove(t);
    delete link;
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
    if (prioritize_thread)
        prioritize_thread->analyze_borrowed_priority(t, this);
}

void Synchronizer_Common::remove_all_lent_priorities() {
    for (auto i = _running_queue.begin(); i != _running_queue.end(); i++) {
        i->object()->analyze_remove_borrowed_priority(this);
    }
    for (auto i = _running_queue.begin(); i != _running_queue.end();) {
        auto next = i->next();
        delete i;
        i = next;
    }
}



__END_SYS