#ifndef LP_DECISION_FRAME_ID_HASH_HPP
#define LP_DECISION_FRAME_ID_HASH_HPP

#include <cstddef>
#include "value_objects/lp_decision_frame_id.hpp"

struct lp_decision_frame_id_hash {
    size_t operator()(const lp_decision_frame_id& id) const noexcept;
};

#endif
