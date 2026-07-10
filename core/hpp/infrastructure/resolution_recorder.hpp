#ifndef RESOLUTION_RECORDER_HPP
#define RESOLUTION_RECORDER_HPP

#include "value_objects/lineage.hpp"

// Minimal recorder for the restarting (non-dbuct) solvers. run_sim exposes a
// single recorder slot with two entry points; the restarting stacks only need
// their decision_memory and resolution_memory updated, so this just fans out:
//   * a decision resolution updates both memories,
//   * a unit resolution updates only the resolution memory.
template<typename IRecordDecision, typename IRecordResolution>
struct resolution_recorder {
    resolution_recorder(IRecordDecision& dm, IRecordResolution& rm)
        : decision_memory_(dm), resolution_memory_(rm) {}

    void record_decision_resolution(const resolution_lineage* rl) {
        decision_memory_.record_decision(rl);
        resolution_memory_.record_resolution(rl);
    }

    void record_unit_resolution(const resolution_lineage* rl) {
        resolution_memory_.record_resolution(rl);
    }

private:
    IRecordDecision& decision_memory_;
    IRecordResolution& resolution_memory_;
};

#endif
