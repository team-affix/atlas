#ifndef SRT_ACTIVE_GOALS_HEURISTICS_ACTION_HPP
#define SRT_ACTIVE_GOALS_HEURISTICS_ACTION_HPP

#include <variant>
#include "value_objects/srt_heuristics_pending_clear.hpp"
#include "value_objects/srt_heuristics_pending_insert.hpp"
#include "value_objects/srt_heuristics_score_assign.hpp"
#include "value_objects/srt_heuristics_score_insert.hpp"
#include "value_objects/srt_heuristics_scores_clear.hpp"

using srt_active_goals_heuristics_action = std::variant<
    srt_heuristics_score_insert,
    srt_heuristics_score_assign,
    srt_heuristics_scores_clear,
    srt_heuristics_pending_insert,
    srt_heuristics_pending_clear>;

#endif
