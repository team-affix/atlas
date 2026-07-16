#ifndef CHOSEN_CANDIDATE_ACTION_HPP
#define CHOSEN_CANDIDATE_ACTION_HPP

#include <variant>
#include "value_objects/chosen_candidate_insert.hpp"
#include "value_objects/chosen_candidate_assign.hpp"

using chosen_candidate_action = std::variant<chosen_candidate_insert, chosen_candidate_assign>;

#endif
