#ifndef DBUCT_DECISION_FRAMES_HPP
#define DBUCT_DECISION_FRAMES_HPP

#include <cstddef>
#include "value_objects/lineage.hpp"

template<typename ILogTrailAction>
struct dbuct_decision_frames {
    dbuct_decision_frames(ILogTrailAction& t);

    void set_decision_frame(const resolution_lineage* rl, size_t frame);

private:
    ILogTrailAction& trail_;
};

template<typename ILogTrailAction>
dbuct_decision_frames<ILogTrailAction>::dbuct_decision_frames(ILogTrailAction& t) : trail_(t) {}

template<typename ILogTrailAction>
void dbuct_decision_frames<ILogTrailAction>::set_decision_frame(const resolution_lineage* rl, size_t frame) {}

#endif
