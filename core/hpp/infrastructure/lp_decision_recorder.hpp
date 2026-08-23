#ifndef LP_DECISION_RECORDER_HPP
#define LP_DECISION_RECORDER_HPP

#include "value_objects/lineage.hpp"

// Hooks frame descent and entry onto decision resolutions.
//
// run_sim exposes two independent recorder slots. This wrapper takes the
// decision slot and the plain resolution_recorder keeps the unit slot, so only
// decisions reach descend() -- which is exactly the property that makes a
// decision detectable. Descent delivers into the child's mailbox and entry
// merges it, and both land immediately before run_sim calls constrain(), so the
// child is armed and its forced eliminations are staged in time to be routed.
template<typename IRecordDecisionResolution, typename IDescendToChildDecisionFrame,
         typename IEnterDecisionFrame>
struct lp_decision_recorder {
    lp_decision_recorder(IRecordDecisionResolution&, IDescendToChildDecisionFrame&,
                         IEnterDecisionFrame&);

    void record_decision_resolution(const resolution_lineage* rl);

private:
    IRecordDecisionResolution& record_decision_resolution_;
    IDescendToChildDecisionFrame& descend_to_child_decision_frame_;
    IEnterDecisionFrame& enter_decision_frame_;
};

template<typename IRDR, typename IDCDF, typename IEDF>
lp_decision_recorder<IRDR, IDCDF, IEDF>::lp_decision_recorder(IRDR& rdr, IDCDF& dcdf,
                                                              IEDF& edf)
    : record_decision_resolution_(rdr)
    , descend_to_child_decision_frame_(dcdf)
    , enter_decision_frame_(edf) {}

template<typename IRDR, typename IDCDF, typename IEDF>
void lp_decision_recorder<IRDR, IDCDF, IEDF>::record_decision_resolution(
    const resolution_lineage* rl) {
    record_decision_resolution_.record_decision_resolution(rl);
    descend_to_child_decision_frame_.descend(rl);
    enter_decision_frame_.enter();
}

#endif
