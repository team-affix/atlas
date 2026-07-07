#ifndef DBUCT_NEAREST_DECISION_HPP
#define DBUCT_NEAREST_DECISION_HPP

#include <unordered_map>
#include "debug_assert.hpp"
#include "value_objects/lineage.hpp"

template<typename ILogTrailAction>
struct dbuct_nearest_decision {
    explicit dbuct_nearest_decision(ILogTrailAction& t);

    void note_unit_resolution(const resolution_lineage* rl);
    void note_decision_resolution(const resolution_lineage* rl);

private:
    std::unordered_map<const resolution_lineage*, const resolution_lineage*> nd_;

    ILogTrailAction& trail_;
};

template<typename ILogTrailAction>
dbuct_nearest_decision<ILogTrailAction>::dbuct_nearest_decision(ILogTrailAction& t) : trail_(t) {}

template<typename ILogTrailAction>
void dbuct_nearest_decision<ILogTrailAction>::note_unit_resolution(const resolution_lineage* rl) {
    auto [_, inserted] = nd_.insert({rl, nd_.at(rl->parent->parent)});
    DEBUG_ASSERT(inserted);
}

template<typename ILogTrailAction>
void dbuct_nearest_decision<ILogTrailAction>::note_decision_resolution(const resolution_lineage* rl) {
    auto [_, inserted] = nd_.insert({rl, rl});
    DEBUG_ASSERT(inserted);
}

#endif
