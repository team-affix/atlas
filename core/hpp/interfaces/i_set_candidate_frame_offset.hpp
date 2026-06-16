#ifndef I_SET_CANDIDATE_FRAME_OFFSET_HPP
#define I_SET_CANDIDATE_FRAME_OFFSET_HPP

#include <cstdint>
#include "value_objects/lineage.hpp"

struct i_set_candidate_frame_offset {
    virtual ~i_set_candidate_frame_offset() = default;
    virtual void set(const resolution_lineage*, uint32_t frame_offset) = 0;
};

#endif
