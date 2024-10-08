// EPOS Heap Utility Implementation

#include <utility/heap.h>

extern "C" { void _panic(); }

__BEGIN_UTIL

Simple_Spin Heap::_spin;
bool Heap::_not_booting = false;
bool Heap::_has_changed_interruption = false;

void Heap::out_of_memory(unsigned long bytes)
{
    db<Heaps, System>(ERR) << "Heap::alloc(this=" << this << "): out of memory while allocating " << bytes << " bytes!" << endl;

    _panic();
}

__END_UTIL
