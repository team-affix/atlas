#ifndef PUD_MODULE_AUTOSAVER_HPP
#define PUD_MODULE_AUTOSAVER_HPP

#include <cstddef>

// Saves the session's modules every unfolds_per_save unfolds and once more when the session ends. With
// unfolds_per_save == 0 it never saves. A due save waits while a solution is pending (derived but not yet
// reported): a saved solved leaf counts as already reported when the modules are loaded again.
template<typename IGetUnfoldCount, typename ISaveAllModules, typename IHasPendingSolution>
struct pud_module_autosaver {
    pud_module_autosaver(
        IGetUnfoldCount& get_unfold_count,
        ISaveAllModules& save_all_modules,
        IHasPendingSolution& has_pending_solution,
        size_t unfolds_per_save);
    void try_save();
    void try_save_final();
private:
    IGetUnfoldCount& get_unfold_count_;
    ISaveAllModules& save_all_modules_;
    IHasPendingSolution& has_pending_solution_;
    size_t unfolds_per_save_;
    size_t unfolds_at_last_save_;
};

template<typename IGetUnfoldCount, typename ISaveAllModules, typename IHasPendingSolution>
pud_module_autosaver<IGetUnfoldCount, ISaveAllModules, IHasPendingSolution>::pud_module_autosaver(
    IGetUnfoldCount& get_unfold_count,
    ISaveAllModules& save_all_modules,
    IHasPendingSolution& has_pending_solution,
    size_t unfolds_per_save)
    : get_unfold_count_(get_unfold_count)
    , save_all_modules_(save_all_modules)
    , has_pending_solution_(has_pending_solution)
    , unfolds_per_save_(unfolds_per_save)
    , unfolds_at_last_save_(0) {}

template<typename IGetUnfoldCount, typename ISaveAllModules, typename IHasPendingSolution>
void pud_module_autosaver<IGetUnfoldCount, ISaveAllModules, IHasPendingSolution>::try_save() {
    if (unfolds_per_save_ == 0)
        return;
    const size_t unfolds = get_unfold_count_.unfold_count();
    const bool due = unfolds - unfolds_at_last_save_ >= unfolds_per_save_;
    if (!due)
        return;
    if (has_pending_solution_.has_pending_solution())
        return;
    save_all_modules_.save_all();
    unfolds_at_last_save_ = unfolds;
}

template<typename IGetUnfoldCount, typename ISaveAllModules, typename IHasPendingSolution>
void pud_module_autosaver<IGetUnfoldCount, ISaveAllModules, IHasPendingSolution>::try_save_final() {
    if (unfolds_per_save_ == 0)
        return;
    save_all_modules_.save_all();
    unfolds_at_last_save_ = get_unfold_count_.unfold_count();
}

#endif
