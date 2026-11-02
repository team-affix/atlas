#include "infrastructure/pud_lodestar_pending_conflict_store.hpp"

pud_lodestar_pending_conflict_store::pud_lodestar_pending_conflict_store()
    : pending_conflicts_(0) {}

size_t pud_lodestar_pending_conflict_store::get() const {
    return pending_conflicts_;
}

void pud_lodestar_pending_conflict_store::set(size_t pending_conflicts) {
    pending_conflicts_ = pending_conflicts;
}
