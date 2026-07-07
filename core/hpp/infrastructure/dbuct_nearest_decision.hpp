#ifndef DBUCT_NEAREST_DECISION_HPP
#define DBUCT_NEAREST_DECISION_HPP

#include <memory>
#include <unordered_map>
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/tracked.hpp"
#include "value_objects/lineage.hpp"

template<typename ILogTrailAction>
struct dbuct_nearest_decision {
    dbuct_nearest_decision(ILogTrailAction& t);

    void note_unit_resolution(const resolution_lineage* rl);
    void note_decision_resolution(const resolution_lineage* rl);

    const resolution_lineage* get_nearest_decision(const resolution_lineage* rl) const;

private:
    using map_t = std::unordered_map<const resolution_lineage*, const resolution_lineage*>;

    tracked<map_t, ILogTrailAction> nd_;
};

template<typename ILogTrailAction>
dbuct_nearest_decision<ILogTrailAction>::dbuct_nearest_decision(ILogTrailAction& t) : nd_(t, map_t{}) {}

template<typename ILogTrailAction>
void dbuct_nearest_decision<ILogTrailAction>::note_unit_resolution(const resolution_lineage* rl) {
    nd_.mutate(std::make_unique<backtrackable_map_insert<map_t>>(rl, nd_.get().at(rl->parent->parent)));
}

template<typename ILogTrailAction>
void dbuct_nearest_decision<ILogTrailAction>::note_decision_resolution(const resolution_lineage* rl) {
    nd_.mutate(std::make_unique<backtrackable_map_insert<map_t>>(rl, rl));
}

template<typename ILogTrailAction>
const resolution_lineage* dbuct_nearest_decision<ILogTrailAction>::get_nearest_decision(const resolution_lineage* rl) const {
    return nd_.get().at(rl);
}

#endif
