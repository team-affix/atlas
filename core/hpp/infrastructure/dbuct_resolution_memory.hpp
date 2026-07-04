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

    void record_resolution(const resolution_lineage* rl);
    void clear_recorded_resolutions();
    size_t get_resolution_count() const;
    lemma derive_resolution_lemma() const;

    snapshot_t snapshot() const;
    void restore(snapshot_t s);

private:
    std::unordered_set<const resolution_lineage*> resolutions;
};

inline void dbuct_resolution_memory::record_resolution(const resolution_lineage* rl) { resolutions.insert(rl); }
inline void dbuct_resolution_memory::clear_recorded_resolutions() { resolutions.clear(); }
inline size_t dbuct_resolution_memory::get_resolution_count() const { return resolutions.size(); }
inline lemma dbuct_resolution_memory::derive_resolution_lemma() const { return lemma{resolutions}; }

inline dbuct_resolution_memory::snapshot_t dbuct_resolution_memory::snapshot() const { return resolutions; }
inline void dbuct_resolution_memory::restore(snapshot_t s) { resolutions = std::move(s); }

#endif
