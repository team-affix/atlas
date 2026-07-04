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

    void record_decision(const resolution_lineage* rl);
    void clear_recorded_decisions();
    size_t count() const;
    lemma derive_decision_lemma() const;

    snapshot_t snapshot() const;
    void restore(snapshot_t s);

private:
    std::unordered_set<const resolution_lineage*> decisions;
};

inline void dbuct_decision_memory::record_decision(const resolution_lineage* rl) { decisions.insert(rl); }

inline void dbuct_decision_memory::clear_recorded_decisions() { decisions.clear(); }

inline size_t dbuct_decision_memory::count() const { return decisions.size(); }

inline lemma dbuct_decision_memory::derive_decision_lemma() const { return lemma{decisions}; }

inline dbuct_decision_memory::snapshot_t dbuct_decision_memory::snapshot() const { return decisions; }
inline void dbuct_decision_memory::restore(snapshot_t s) { decisions = std::move(s); }

#endif
