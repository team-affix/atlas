#ifndef I_UNSET_CANDIDATE_FRAME_OFFSET_HPP
#define I_UNSET_CANDIDATE_FRAME_OFFSET_HPP

#include "value_objects/lineage.hpp"

struct i_unset_candidate_frame_offset {
    virtual ~i_unset_candidate_frame_offset() = default;
    virtual void unset(const resolution_lineage*) = 0;
};

#endif
