#include "infrastructure/pud_lodestar_pending_conflict_store.hpp"

pud_lodestar_pending_conflict_store::pud_lodestar_pending_conflict_store()
    : pending_conflicts_(0) {}

void pud_lodestar_pending_conflict_store::add(size_t conflicts) {
    pending_conflicts_ += conflicts;
}

bool pud_lodestar_pending_conflict_store::try_take() {
    if (pending_conflicts_ == 0)
        return false;
    --pending_conflicts_;
    return true;
}
