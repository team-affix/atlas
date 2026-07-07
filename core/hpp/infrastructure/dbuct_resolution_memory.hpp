#ifndef DBUCT_RESOLUTION_MEMORY_HPP
#define DBUCT_RESOLUTION_MEMORY_HPP

#include <memory>
#include <unordered_set>
#include "infrastructure/backtrackable_set_insert.hpp"
#include "infrastructure/tracked.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/lemma.hpp"

// Delayed-backtracking variant of resolution_memory. Trail-journalled (via
// ILogTrailAction) so get_resolution_count() reflects the resolutions on the
// active (camped) path: a choice-frame pop removes exactly those recorded since.
template<typename ILogTrailAction>
struct dbuct_resolution_memory {
    explicit dbuct_resolution_memory(ILogTrailAction& t);

    void record_resolution(const resolution_lineage* rl);
    size_t get_resolution_count() const;
    lemma derive_resolution_lemma() const;

private:
    using set_t = std::unordered_set<const resolution_lineage*>;

    tracked<set_t, ILogTrailAction> resolutions_;
};

template<typename ILogTrailAction>
dbuct_resolution_memory<ILogTrailAction>::dbuct_resolution_memory(ILogTrailAction& t) : resolutions_(t, set_t{}) {}

template<typename ILogTrailAction>
void dbuct_resolution_memory<ILogTrailAction>::record_resolution(const resolution_lineage* rl) {
    if (resolutions_.get().contains(rl)) return;
    resolutions_.mutate(std::make_unique<backtrackable_set_insert<set_t>>(rl));
}

template<typename ILogTrailAction>
size_t dbuct_resolution_memory<ILogTrailAction>::get_resolution_count() const { return resolutions_.get().size(); }

template<typename ILogTrailAction>
lemma dbuct_resolution_memory<ILogTrailAction>::derive_resolution_lemma() const { return lemma{resolutions_.get()}; }

#endif
