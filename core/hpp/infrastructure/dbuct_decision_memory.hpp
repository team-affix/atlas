#ifndef DBUCT_DECISION_MEMORY_HPP
#define DBUCT_DECISION_MEMORY_HPP

#include <memory>
#include <unordered_set>
#include "infrastructure/backtrackable_set_insert.hpp"
#include "infrastructure/tracked.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/lemma.hpp"

// Delayed-backtracking variant of decision_memory. Not cleared per sim: it
// accumulates the decisions along the active (camped) path (so count() and
// derive_decision_lemma() are full-path), journalled on the trail (via
// ILogTrailAction) so a choice-frame pop rolls it back exactly.
template<typename ILogTrailAction>
struct dbuct_decision_memory {
    using set_t = std::unordered_set<const resolution_lineage*>;

    explicit dbuct_decision_memory(ILogTrailAction& t);

    void record_decision(const resolution_lineage* rl);
    size_t count() const;
    lemma derive_decision_lemma() const;

private:
    tracked<set_t, ILogTrailAction> decisions_;
};

template<typename ILogTrailAction>
dbuct_decision_memory<ILogTrailAction>::dbuct_decision_memory(ILogTrailAction& t) : decisions_(t, set_t{}) {}

template<typename ILogTrailAction>
void dbuct_decision_memory<ILogTrailAction>::record_decision(const resolution_lineage* rl) {
    if (decisions_.get().contains(rl)) return;
    decisions_.mutate(std::make_unique<backtrackable_set_insert<set_t>>(rl));
}

template<typename ILogTrailAction>
size_t dbuct_decision_memory<ILogTrailAction>::count() const { return decisions_.get().size(); }

template<typename ILogTrailAction>
lemma dbuct_decision_memory<ILogTrailAction>::derive_decision_lemma() const { return lemma{decisions_.get()}; }

#endif
