#ifndef I_CANDIDATE_NOT_APPLICABLE_DETECTOR_HPP
#define I_CANDIDATE_NOT_APPLICABLE_DETECTOR_HPP

#include <cstdint>
#include "../value_objects/lineage.hpp"

struct i_candidate_not_applicable_detector {
    virtual ~i_candidate_not_applicable_detector() = default;
    virtual void representative_changed(uint32_t) = 0;
    virtual void goal_activated(const goal_lineage*) = 0;
    virtual void goal_deactivated(const goal_lineage*) = 0;
};

#endif
