#ifndef PUD_LODESTAR_PENDING_CONFLICT_STORE_HPP
#define PUD_LODESTAR_PENDING_CONFLICT_STORE_HPP

#include <cstddef>

struct pud_lodestar_pending_conflict_store {
    pud_lodestar_pending_conflict_store();
    size_t get() const;
    void set(size_t pending_conflicts);
private:
    size_t pending_conflicts_;
};

#endif
