#ifndef DBUCT_DECISION_MEMORY_HPP
#define DBUCT_DECISION_MEMORY_HPP

#include <unordered_set>
#include "value_objects/lineage.hpp"
#include "value_objects/lemma.hpp"

// Delayed-backtracking variant of decision_memory.
//
// Under DBUCT this is NOT cleared per sim: it accumulates the resolution
// decisions along the currently active (camped) path from the root, so count()
// yields the full-path decision count used by the reward, and
// derive_decision_lemma() yields the full-path conflict lemma. Checkpoint
// restore rolls it back exactly to any choice boundary.
struct dbuct_decision_memory {
    using snapshot_t = std::unordered_set<const resolution_lineage*>;

    void record_decision(const resolution_lineage* rl) { decisions.insert(rl); }

    void clear_recorded_decisions() { decisions.clear(); }

    size_t count() const { return decisions.size(); }

    lemma derive_decision_lemma() const { return lemma{decisions}; }

    snapshot_t snapshot() const { return decisions; }
    void restore(snapshot_t s) { decisions = std::move(s); }

private:
    std::unordered_set<const resolution_lineage*> decisions;
};

#endif
