#ifndef RP_SRT_ACTIVE_GOALS_ACTION_HPP
#define RP_SRT_ACTIVE_GOALS_ACTION_HPP

#include <variant>
#include "value_objects/rp_srt_score_assign.hpp"
#include "value_objects/rp_srt_score_insert.hpp"

using rp_srt_active_goals_action = std::variant<
    rp_srt_score_insert,
    rp_srt_score_assign>;

#endif
