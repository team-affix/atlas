#ifndef PUD_LODESTAR_UNFOLD_BUDGET_STORE_HPP
#define PUD_LODESTAR_UNFOLD_BUDGET_STORE_HPP

#include <cstddef>

struct pud_lodestar_unfold_budget_store {
    pud_lodestar_unfold_budget_store();
    size_t get_focus_unfolds() const;
    void set_focus_unfolds(size_t focus_unfolds);
    bool get_stopped() const;
    void set_stopped(bool stopped);
private:
    size_t focus_unfolds_;
    bool stopped_;
};

#endif
