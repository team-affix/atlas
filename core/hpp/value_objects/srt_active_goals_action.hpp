#ifndef SRT_ACTIVE_GOALS_ACTION_HPP
#define SRT_ACTIVE_GOALS_ACTION_HPP

#include <variant>
#include "value_objects/srt_in_flight_clear.hpp"
#include "value_objects/srt_in_flight_insert.hpp"

using srt_active_goals_action = std::variant<srt_in_flight_insert, srt_in_flight_clear>;

#endif
