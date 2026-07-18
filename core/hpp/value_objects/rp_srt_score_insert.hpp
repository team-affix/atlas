#ifndef RP_SRT_SCORE_INSERT_HPP
#define RP_SRT_SCORE_INSERT_HPP

#include <compare>
#include "value_objects/lineage.hpp"

struct rp_srt_score_insert {
    const goal_lineage* gl;
    auto operator<=>(const rp_srt_score_insert&) const = default;
};

#endif
