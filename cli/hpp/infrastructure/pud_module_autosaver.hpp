#ifndef PUD_MODULE_AUTOSAVER_HPP
#define PUD_MODULE_AUTOSAVER_HPP

#include <cstddef>

// Saves the session's modules every unfolds_per_save unfolds and once more when the session ends. With
// unfolds_per_save == 0 it never saves.
template<typename IGetUnfoldCount, typename ISaveAllModules>
struct pud_module_autosaver {
    pud_module_autosaver(
        IGetUnfoldCount& get_unfold_count,
        ISaveAllModules& save_all_modules,
        size_t unfolds_per_save);
    void try_save();
    void try_save_final();
private:
    IGetUnfoldCount& get_unfold_count_;
    ISaveAllModules& save_all_modules_;
    size_t unfolds_per_save_;
    size_t unfolds_at_last_save_;
};

template<typename IGetUnfoldCount, typename ISaveAllModules>
pud_module_autosaver<IGetUnfoldCount, ISaveAllModules>::pud_module_autosaver(
    IGetUnfoldCount& get_unfold_count,
    ISaveAllModules& save_all_modules,
    size_t unfolds_per_save)
    : get_unfold_count_(get_unfold_count)
    , save_all_modules_(save_all_modules)
    , unfolds_per_save_(unfolds_per_save)
    , unfolds_at_last_save_(0) {}

template<typename IGetUnfoldCount, typename ISaveAllModules>
void pud_module_autosaver<IGetUnfoldCount, ISaveAllModules>::try_save() {
    if (unfolds_per_save_ == 0)
        return;
    const size_t unfolds = get_unfold_count_.unfold_count();
    const bool due = unfolds - unfolds_at_last_save_ >= unfolds_per_save_;
    if (!due)
        return;
    save_all_modules_.save_all();
    unfolds_at_last_save_ = unfolds;
}

template<typename IGetUnfoldCount, typename ISaveAllModules>
void pud_module_autosaver<IGetUnfoldCount, ISaveAllModules>::try_save_final() {
    if (unfolds_per_save_ == 0)
        return;
    save_all_modules_.save_all();
    unfolds_at_last_save_ = get_unfold_count_.unfold_count();
}

#endif
