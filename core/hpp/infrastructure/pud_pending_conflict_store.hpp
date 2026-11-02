#ifndef PUD_PENDING_CONFLICT_STORE_HPP
#define PUD_PENDING_CONFLICT_STORE_HPP

#include <cstddef>

// Counts refutations that module registration produced and that have not yet been reported as ticks.
struct pud_pending_conflict_store {
    pud_pending_conflict_store();
    void add(size_t conflicts);
    bool try_take();
private:
    size_t pending_conflicts_;
};

#endif
