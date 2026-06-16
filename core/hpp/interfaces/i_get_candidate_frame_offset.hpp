#ifndef I_GET_CANDIDATE_FRAME_OFFSET_HPP
#define I_GET_CANDIDATE_FRAME_OFFSET_HPP

#include <cstdint>
#include "value_objects/lineage.hpp"

struct i_get_candidate_frame_offset {
    virtual ~i_get_candidate_frame_offset() = default;
    virtual uint32_t get(const resolution_lineage*) const = 0;
};

#endif
