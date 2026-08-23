#ifndef LP_DECISION_RECORDER_HPP
#define LP_DECISION_RECORDER_HPP

#include "value_objects/lineage.hpp"

// Hooks frame descent onto decision resolutions.
//
// run_sim exposes two independent recorder slots. This wrapper takes the
// decision slot and the plain resolution_recorder keeps the unit slot, so only
// decisions reach descend() -- which is exactly the property that makes a
// decision detectable. It also lands the descent immediately before run_sim
// calls constrain(), which is where the descent's forced eliminations get
// routed.
template<typename IRecordDecisionResolution, typename IDescendToChildDecisionFrame>
struct lp_decision_recorder {
    lp_decision_recorder(IRecordDecisionResolution&, IDescendToChildDecisionFrame&);

    void record_decision_resolution(const resolution_lineage* rl);

private:
    IRecordDecisionResolution& record_decision_resolution_;
    IDescendToChildDecisionFrame& descend_to_child_decision_frame_;
};

template<typename IRDR, typename IDCDF>
lp_decision_recorder<IRDR, IDCDF>::lp_decision_recorder(IRDR& rdr, IDCDF& dcdf)
    : record_decision_resolution_(rdr), descend_to_child_decision_frame_(dcdf) {}

template<typename IRDR, typename IDCDF>
void lp_decision_recorder<IRDR, IDCDF>::record_decision_resolution(
    const resolution_lineage* rl) {
    record_decision_resolution_.record_decision_resolution(rl);
    descend_to_child_decision_frame_.descend(rl);
}

#endif
