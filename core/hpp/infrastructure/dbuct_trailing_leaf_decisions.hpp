#ifndef DBUCT_TRAILING_LEAF_DECISIONS_HPP
#define DBUCT_TRAILING_LEAF_DECISIONS_HPP

#include "value_objects/lineage.hpp"

template<typename ILogTrailAction>
struct dbuct_trailing_leaf_decisions {
    dbuct_trailing_leaf_decisions(ILogTrailAction& t);

    void log_decision(const resolution_lineage* rl);

private:
    const resolution_lineage* ultimate_;
    const resolution_lineage* penultimate_;

    ILogTrailAction& trail_;
};

template<typename ILogTrailAction>
dbuct_trailing_leaf_decisions<ILogTrailAction>::dbuct_trailing_leaf_decisions(ILogTrailAction& t) : trail_(t) {}

template<typename ILogTrailAction>
void dbuct_trailing_leaf_decisions<ILogTrailAction>::log_decision(const resolution_lineage* rl) {}

#endif
