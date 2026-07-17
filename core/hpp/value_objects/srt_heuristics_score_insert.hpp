#ifndef SRT_HEURISTICS_SCORE_INSERT_HPP
#define SRT_HEURISTICS_SCORE_INSERT_HPP

#include <compare>
#include "value_objects/lineage.hpp"

struct srt_heuristics_score_insert {
    const goal_lineage* gl;
    auto operator<=>(const srt_heuristics_score_insert&) const = default;
};

#endif
