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
         typename INearestDecision, typename ILogDecision>
struct dbuct_resolution_recorder {
    dbuct_resolution_recorder(IRecordDecision& dm, IRecordResolution& rm,
                              INearestDecision& nd, ILogDecision& aub)
        : decision_memory_(dm), resolution_memory_(rm),
          nearest_decision_(nd), avoidance_unit_boundary_(aub) {}

    void record_decision_resolution(const resolution_lineage* rl) {
        decision_memory_.record_decision(rl);
        nearest_decision_.note_decision_resolution(rl);
        avoidance_unit_boundary_.log_decision(rl);
        record_resolution(rl);
    }

    void record_unit_resolution(const resolution_lineage* rl) {
        record_resolution(rl);
        nearest_decision_.note_unit_resolution(rl);
    }

private:
    void record_resolution(const resolution_lineage* rl) {
        resolution_memory_.record_resolution(rl);
    }

    IRecordDecision& decision_memory_;
    IRecordResolution& resolution_memory_;
    INearestDecision& nearest_decision_;
    ILogDecision& avoidance_unit_boundary_;
};

#endif
