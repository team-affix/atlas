#ifndef DBUCT_DECISION_MEMORY_HPP
#define DBUCT_DECISION_MEMORY_HPP

#include <memory>
#include <unordered_set>
#include "infrastructure/backtrackable_set_insert.hpp"
#include "infrastructure/tracked.hpp"
#include "infrastructure/trail.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/lemma.hpp"

// Delayed-backtracking variant of decision_memory.
//
// Under DBUCT this is NOT cleared per sim: it accumulates the resolution
// decisions along the currently active (camped) path from the root, so count()
// yields the full-path decision count used by the reward, and
// derive_decision_lemma() yields the full-path conflict lemma. Its inserts are
// journalled on the trail, so a choice-frame pop rolls it back exactly.
struct dbuct_decision_memory {
    using set_t = std::unordered_set<const resolution_lineage*>;

    explicit dbuct_decision_memory(trail& t);

    void record_decision(const resolution_lineage* rl);
    size_t count() const;
    lemma derive_decision_lemma() const;

private:
    tracked<set_t, trail> decisions_;
};

inline dbuct_decision_memory::dbuct_decision_memory(trail& t) : decisions_(t, set_t{}) {}

inline void dbuct_decision_memory::record_decision(const resolution_lineage* rl) {
    if (decisions_.get().contains(rl)) return;
    decisions_.mutate(std::make_unique<backtrackable_set_insert<set_t>>(rl));
}

inline size_t dbuct_decision_memory::count() const { return decisions_.get().size(); }

inline lemma dbuct_decision_memory::derive_decision_lemma() const { return lemma{decisions_.get()}; }

#endif
