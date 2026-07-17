#ifndef SRT_HEURISTICS_SCORE_ASSIGN_HPP
#define SRT_HEURISTICS_SCORE_ASSIGN_HPP

#include <compare>
#include "value_objects/lineage.hpp"

struct srt_heuristics_score_assign {
    const goal_lineage* gl;
    double previous;
    auto operator<=>(const srt_heuristics_score_assign&) const = default;
};

#endif
