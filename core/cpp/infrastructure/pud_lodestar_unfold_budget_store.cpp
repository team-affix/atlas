#include "infrastructure/pud_lodestar_unfold_budget_store.hpp"

pud_lodestar_unfold_budget_store::pud_lodestar_unfold_budget_store(size_t max_unfolds)
    : max_unfolds_(max_unfolds)
    , unfolds_(0)
    , focus_unfolds_(0)
    , stopped_(false) {}

size_t pud_lodestar_unfold_budget_store::get_max_unfolds() const {
    return max_unfolds_;
}

size_t pud_lodestar_unfold_budget_store::get_unfolds() const {
    return unfolds_;
}

void pud_lodestar_unfold_budget_store::set_unfolds(size_t unfolds) {
    unfolds_ = unfolds;
}

size_t pud_lodestar_unfold_budget_store::get_focus_unfolds() const {
    return focus_unfolds_;
}

void pud_lodestar_unfold_budget_store::set_focus_unfolds(size_t focus_unfolds) {
    focus_unfolds_ = focus_unfolds;
}

bool pud_lodestar_unfold_budget_store::get_stopped() const {
    return stopped_;
}

void pud_lodestar_unfold_budget_store::set_stopped(bool stopped) {
    stopped_ = stopped;
}
