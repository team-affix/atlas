#ifndef RP_SRT_SCORE_ASSIGN_HPP
#define RP_SRT_SCORE_ASSIGN_HPP

#include <compare>
#include "value_objects/lineage.hpp"

struct rp_srt_score_assign {
    const goal_lineage* gl;
    double previous;
    auto operator<=>(const rp_srt_score_assign&) const = default;
};

#endif
