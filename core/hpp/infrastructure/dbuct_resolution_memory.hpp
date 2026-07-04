#ifndef DBUCT_RESOLUTION_MEMORY_HPP
#define DBUCT_RESOLUTION_MEMORY_HPP

#include <unordered_set>
#include "value_objects/lineage.hpp"
#include "value_objects/lemma.hpp"

// Delayed-backtracking variant of resolution_memory. Checkpointed so that
// resolution_depth() reflects the resolutions on the currently active (camped)
// path rather than a lifetime-cumulative count.
struct dbuct_resolution_memory {
    using snapshot_t = std::unordered_set<const resolution_lineage*>;

    void record_resolution(const resolution_lineage* rl) { resolutions.insert(rl); }
    void clear_recorded_resolutions() { resolutions.clear(); }
    size_t get_resolution_count() const { return resolutions.size(); }
    lemma derive_resolution_lemma() const { return lemma{resolutions}; }

    snapshot_t snapshot() const { return resolutions; }
    void restore(snapshot_t s) { resolutions = std::move(s); }

private:
    std::unordered_set<const resolution_lineage*> resolutions;
};

#endif
