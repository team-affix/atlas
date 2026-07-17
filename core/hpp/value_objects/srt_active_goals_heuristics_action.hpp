#ifndef SRT_ACTIVE_GOALS_HEURISTICS_ACTION_HPP
#define SRT_ACTIVE_GOALS_HEURISTICS_ACTION_HPP

#include <variant>
#include "value_objects/srt_heuristics_score_assign.hpp"
#include "value_objects/srt_heuristics_score_insert.hpp"

using srt_active_goals_heuristics_action = std::variant<
    srt_heuristics_score_insert,
    srt_heuristics_score_assign>;

#endif
