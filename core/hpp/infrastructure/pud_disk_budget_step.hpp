#ifndef PUD_DISK_BUDGET_STEP_HPP
#define PUD_DISK_BUDGET_STEP_HPP

#include <cstddef>
#include "value_objects/pud_unfold_outcome.hpp"

// Makes one search step while the modules' files fit their disk budget. Once they reach it, no step is made and the
// budget remembers that it ran out, so the session can say why it stopped.
template<typename IUnfoldOne, typename IMeasureDiskBytes>
struct pud_disk_budget_step {
    pud_disk_budget_step(IUnfoldOne& unfold_one, IMeasureDiskBytes& measure_disk_bytes, size_t max_disk_bytes);
    pud_unfold_outcome unfold_one();
    bool exceeded() const;
private:
    IUnfoldOne& unfold_one_;
    IMeasureDiskBytes& measure_disk_bytes_;
    size_t max_disk_bytes_;
    bool exceeded_;
};

template<typename IUnfoldOne, typename IMeasureDiskBytes>
pud_disk_budget_step<IUnfoldOne, IMeasureDiskBytes>::pud_disk_budget_step(
    IUnfoldOne& unfold_one, IMeasureDiskBytes& measure_disk_bytes, size_t max_disk_bytes)
    : unfold_one_(unfold_one)
    , measure_disk_bytes_(measure_disk_bytes)
    , max_disk_bytes_(max_disk_bytes)
    , exceeded_(false) {}

template<typename IUnfoldOne, typename IMeasureDiskBytes>
pud_unfold_outcome pud_disk_budget_step<IUnfoldOne, IMeasureDiskBytes>::unfold_one() {
    exceeded_ = measure_disk_bytes_.bytes() >= max_disk_bytes_;
    if (exceeded_)
        return pud_unfold_outcome::none;
    return unfold_one_.unfold_one();
}

template<typename IUnfoldOne, typename IMeasureDiskBytes>
bool pud_disk_budget_step<IUnfoldOne, IMeasureDiskBytes>::exceeded() const {
    return exceeded_;
}

#endif
