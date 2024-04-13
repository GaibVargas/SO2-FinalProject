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
    for (auto i = _running_queue.begin(); i != _running_queue.end(); i++) {
        i->object()->analyze_borrowed_priority(t, this);
        //ANNOTATION: se o método retornar um bool pra informar caso tenha mudado a prioridade?
        // Assim poderia ser dado break no for e só uma thread teria sua prioridade aumentada
    }
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