#ifndef DBUCT_RESOLUTION_MEMORY_HPP
#define DBUCT_RESOLUTION_MEMORY_HPP

#include <memory>
#include <unordered_set>
#include "infrastructure/backtrackable_set_insert.hpp"
#include "infrastructure/tracked.hpp"
#include "infrastructure/trail.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/lemma.hpp"

// Delayed-backtracking variant of resolution_memory. Trail-journalled so that
// get_resolution_count() reflects the resolutions on the currently active
// (camped) path rather than a lifetime-cumulative count: a choice-frame pop
// removes exactly the resolutions recorded since that frame was opened.
struct dbuct_resolution_memory {
    using set_t = std::unordered_set<const resolution_lineage*>;

    explicit dbuct_resolution_memory(trail& t);

    void record_resolution(const resolution_lineage* rl);
    size_t get_resolution_count() const;
    lemma derive_resolution_lemma() const;

private:
    tracked<set_t, trail> resolutions_;
};

inline dbuct_resolution_memory::dbuct_resolution_memory(trail& t) : resolutions_(t, set_t{}) {}

inline void dbuct_resolution_memory::record_resolution(const resolution_lineage* rl) {
    if (resolutions_.get().contains(rl)) return;
    resolutions_.mutate(std::make_unique<backtrackable_set_insert<set_t>>(rl));
}

inline size_t dbuct_resolution_memory::get_resolution_count() const { return resolutions_.get().size(); }

inline lemma dbuct_resolution_memory::derive_resolution_lemma() const { return lemma{resolutions_.get()}; }

#endif
