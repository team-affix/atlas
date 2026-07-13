#ifndef DBUCT_RESOLUTION_RECORDER_HPP
#define DBUCT_RESOLUTION_RECORDER_HPP

#include "value_objects/lineage.hpp"

// Fans run_sim's per-step recording out to the several dbuct bookkeeping oracles
// that must observe every resolution. run_sim names the two cases explicitly:
//   * record_decision_resolution(rl) for a resolution that opened a decision, and
//   * record_unit_resolution(rl)     for a forced (unit) resolution.
// Every resolution -- decision or unit -- is recorded in resolution_memory via
// the private record_resolution helper.
//
// - decision:  decision_memory.record_decision + nearest_decision.note_decision_resolution
//              + avoidance_unit_boundary.log_decision, then resolution_memory.record_resolution.
// - unit:      resolution_memory.record_resolution + nearest_decision.note_unit_resolution.
template<typename IRecordDecision, typename IRecordResolution,
         typename INoteDecisionResolution, typename INoteUnitResolution, typename ILogDecision>
struct dbuct_resolution_recorder {
    dbuct_resolution_recorder(IRecordDecision& dm, IRecordResolution& rm,
                              INoteDecisionResolution& ndr, INoteUnitResolution& nur,
                              ILogDecision& aub);

    void record_decision_resolution(const resolution_lineage* rl);
    void record_unit_resolution(const resolution_lineage* rl);

private:
    void record_resolution(const resolution_lineage* rl);

    IRecordDecision& decision_memory_;
    IRecordResolution& resolution_memory_;
    INoteDecisionResolution& note_decision_resolution_;
    INoteUnitResolution& note_unit_resolution_;
    ILogDecision& avoidance_unit_boundary_;
};

template<typename IRecordDecision, typename IRecordResolution,
         typename INoteDecisionResolution, typename INoteUnitResolution, typename ILogDecision>
dbuct_resolution_recorder<IRecordDecision, IRecordResolution,
                          INoteDecisionResolution, INoteUnitResolution, ILogDecision>::dbuct_resolution_recorder(
    IRecordDecision& dm, IRecordResolution& rm,
    INoteDecisionResolution& ndr, INoteUnitResolution& nur, ILogDecision& aub)
    : decision_memory_(dm), resolution_memory_(rm),
      note_decision_resolution_(ndr), note_unit_resolution_(nur),
      avoidance_unit_boundary_(aub) {}

template<typename IRecordDecision, typename IRecordResolution,
         typename INoteDecisionResolution, typename INoteUnitResolution, typename ILogDecision>
void dbuct_resolution_recorder<IRecordDecision, IRecordResolution,
                               INoteDecisionResolution, INoteUnitResolution, ILogDecision>::record_decision_resolution(
    const resolution_lineage* rl) {
    decision_memory_.record_decision(rl);
    note_decision_resolution_.note_decision_resolution(rl);
    avoidance_unit_boundary_.log_decision(rl);
    record_resolution(rl);
}

template<typename IRecordDecision, typename IRecordResolution,
         typename INoteDecisionResolution, typename INoteUnitResolution, typename ILogDecision>
void dbuct_resolution_recorder<IRecordDecision, IRecordResolution,
                               INoteDecisionResolution, INoteUnitResolution, ILogDecision>::record_unit_resolution(
    const resolution_lineage* rl) {
    record_resolution(rl);
    note_unit_resolution_.note_unit_resolution(rl);
}

template<typename IRecordDecision, typename IRecordResolution,
         typename INoteDecisionResolution, typename INoteUnitResolution, typename ILogDecision>
void dbuct_resolution_recorder<IRecordDecision, IRecordResolution,
                               INoteDecisionResolution, INoteUnitResolution, ILogDecision>::record_resolution(
    const resolution_lineage* rl) {
    resolution_memory_.record_resolution(rl);
}

#endif
