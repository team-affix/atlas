#ifndef DBUCT_AVOIDANCE_DORMANCY_BOUNDARY_HPP
#define DBUCT_AVOIDANCE_DORMANCY_BOUNDARY_HPP

#include "value_objects/lineage.hpp"

template<typename IGetNearestDecision, typename IGetFrameCount, typename ILogTrailAction>
struct dbuct_avoidance_dormancy_boundary {
    dbuct_avoidance_dormancy_boundary(IGetNearestDecision& nd, IGetFrameCount& fc, ILogTrailAction& t);

    void log_decision(const resolution_lineage* rl);
    size_t get_decision_boundary() const;

private:
    const resolution_lineage* ultimate_;
    size_t ultimate_frame_index_;
    size_t dormancy_frame_index_;

    IGetNearestDecision& get_nearest_decision_;
    IGetFrameCount& get_frame_count_;
    ILogTrailAction& trail_;
};

template<typename IGetNearestDecision, typename IGetFrameCount, typename ILogTrailAction>
dbuct_avoidance_dormancy_boundary<IGetNearestDecision, IGetFrameCount, ILogTrailAction>::dbuct_avoidance_dormancy_boundary(IGetNearestDecision& nd, IGetFrameCount& fc, ILogTrailAction& t)
    : get_nearest_decision_(nd),
    get_frame_count_(fc),
    trail_(t),
    ultimate_(nullptr),
    ultimate_frame_index_(0),
    dormancy_frame_index_(0) {}

template<typename IGetNearestDecision, typename IGetFrameCount, typename ILogTrailAction>
void dbuct_avoidance_dormancy_boundary<IGetNearestDecision, IGetFrameCount, ILogTrailAction>::log_decision(const resolution_lineage* rl) {
    // if ultimate is the decision-parent of rl, just overwrite ultimate (this is because we are
    // trying to progressively match the ancestor-removal behavior of lemma constructor which is
    // used in avoidance creation.) Else, save the previous ultimate frame index as the dormancy frame index.
    if (ultimate_ != get_nearest_decision_.get_nearest_decision(rl->parent->parent)) {
        // otherwise, rotate and overwrite ultimate
        dormancy_frame_index_ = ultimate_frame_index_;
    }

    ultimate_ = rl;
    ultimate_frame_index_ = get_frame_count_.depth();
}

template<typename IGetNearestDecision, typename IGetFrameCount, typename ILogTrailAction>
size_t dbuct_avoidance_dormancy_boundary<IGetNearestDecision, IGetFrameCount, ILogTrailAction>::get_decision_boundary() const {
    return dormancy_frame_index_;
}

#endif
