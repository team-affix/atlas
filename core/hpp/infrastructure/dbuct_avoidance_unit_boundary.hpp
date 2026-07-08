#ifndef DBUCT_AVOIDANCE_UNIT_BOUNDARY_HPP
#define DBUCT_AVOIDANCE_UNIT_BOUNDARY_HPP

#include <memory>
#include "value_objects/lineage.hpp"
#include "infrastructure/tracked.hpp"
#include "infrastructure/backtrackable_assign.hpp"

template<typename IGetNearestDecision, typename IGetFrameCount, typename ILogTrailAction>
struct dbuct_avoidance_unit_boundary {
    dbuct_avoidance_unit_boundary(IGetNearestDecision& nd, IGetFrameCount& fc, ILogTrailAction& t);

    void log_decision(const resolution_lineage* rl);
    size_t get_unit_boundary() const;
    const resolution_lineage* get_ultimate_decision() const;
    const resolution_lineage* get_penultimate_decision() const;

private:
    tracked<const resolution_lineage*, ILogTrailAction> ultimate_;
    tracked<const resolution_lineage*, ILogTrailAction> penultimate_;
    tracked<size_t, ILogTrailAction> ultimate_frame_index_;
    tracked<size_t, ILogTrailAction> unit_boundary_frame_index_;

    IGetNearestDecision& get_nearest_decision_;
    IGetFrameCount& get_frame_count_;
    ILogTrailAction& trail_;
};

template<typename IGetNearestDecision, typename IGetFrameCount, typename ILogTrailAction>
dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetFrameCount, ILogTrailAction>::dbuct_avoidance_unit_boundary(IGetNearestDecision& nd, IGetFrameCount& fc, ILogTrailAction& t)
    : ultimate_(t, nullptr),
    penultimate_(t, nullptr),
    ultimate_frame_index_(t, 0),
    unit_boundary_frame_index_(t, 0),
    get_nearest_decision_(nd),
    get_frame_count_(fc),
    trail_(t) {}

template<typename IGetNearestDecision, typename IGetFrameCount, typename ILogTrailAction>
void dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetFrameCount, ILogTrailAction>::log_decision(const resolution_lineage* rl) {
    // if ultimate is the decision-parent of rl, just overwrite ultimate (this is because we are
    // trying to progressively match the ancestor-removal behavior of lemma constructor which is
    // used in avoidance creation.) Else, save the previous ultimate frame index as the unit boundary frame index.
    if (ultimate_.get() != get_nearest_decision_.get_nearest_decision(rl->parent->parent)) {
        // otherwise, rotate and overwrite ultimate
        penultimate_.mutate(std::make_unique<backtrackable_assign<const resolution_lineage*>>(ultimate_.get()));
        unit_boundary_frame_index_.mutate(std::make_unique<backtrackable_assign<size_t>>(ultimate_frame_index_.get()));
    }

    ultimate_.mutate(std::make_unique<backtrackable_assign<const resolution_lineage*>>(rl));
    ultimate_frame_index_.mutate(std::make_unique<backtrackable_assign<size_t>>(get_frame_count_.depth()));
}

template<typename IGetNearestDecision, typename IGetFrameCount, typename ILogTrailAction>
size_t dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetFrameCount, ILogTrailAction>::get_unit_boundary() const {
    return unit_boundary_frame_index_.get();
}

template<typename IGetNearestDecision, typename IGetFrameCount, typename ILogTrailAction>
const resolution_lineage* dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetFrameCount, ILogTrailAction>::get_ultimate_decision() const {
    return ultimate_.get();
}

template<typename IGetNearestDecision, typename IGetFrameCount, typename ILogTrailAction>
const resolution_lineage* dbuct_avoidance_unit_boundary<IGetNearestDecision, IGetFrameCount, ILogTrailAction>::get_penultimate_decision() const {
    return penultimate_.get();
}

#endif
