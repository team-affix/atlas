#ifndef DBUCT_DECISION_ROUTER_HPP
#define DBUCT_DECISION_ROUTER_HPP

#include "value_objects/lineage.hpp"

// Fans run_sim's record-decision / record-resolution callbacks out to the several
// dbuct bookkeeping oracles that must observe every step, so run_sim keeps its two
// original recorder slots. run_sim calls record_decision(rl) THEN record_resolution(rl)
// for a decision, and only record_resolution(rl) for a unit resolution. We use that
// adjacency to tell decisions from units: a decision is marked pending in
// record_decision and consumed by the immediately-following record_resolution.
//
// - decision:  decision_memory.record_decision + nearest_decision.note_decision_resolution
//              + avoidance_unit_boundary.log_decision, then resolution_memory.record_resolution.
// - unit:      resolution_memory.record_resolution + nearest_decision.note_unit_resolution.
template<typename IRecordDecision, typename IRecordResolution,
         typename INearestDecision, typename ILogDecision>
struct dbuct_decision_router {
    dbuct_decision_router(IRecordDecision& dm, IRecordResolution& rm,
                          INearestDecision& nd, ILogDecision& aub)
        : decision_memory_(dm), resolution_memory_(rm),
          nearest_decision_(nd), avoidance_unit_boundary_(aub) {}

    void record_decision(const resolution_lineage* rl) {
        decision_memory_.record_decision(rl);
        nearest_decision_.note_decision_resolution(rl);
        avoidance_unit_boundary_.log_decision(rl);
        pending_decision_ = rl;
    }

    void record_resolution(const resolution_lineage* rl) {
        resolution_memory_.record_resolution(rl);
        if (rl != pending_decision_)
            nearest_decision_.note_unit_resolution(rl);
        pending_decision_ = nullptr;
    }

private:
    IRecordDecision& decision_memory_;
    IRecordResolution& resolution_memory_;
    INearestDecision& nearest_decision_;
    ILogDecision& avoidance_unit_boundary_;
    // Transient within a single next_resolution() cycle (set by record_decision,
    // consumed by the immediately following record_resolution); never spans a
    // backtrack, so it needs no trail journalling.
    const resolution_lineage* pending_decision_ = nullptr;
};

#endif
