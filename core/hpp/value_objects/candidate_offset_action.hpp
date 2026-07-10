#ifndef CANDIDATE_OFFSET_ACTION_HPP
#define CANDIDATE_OFFSET_ACTION_HPP

#include <variant>
#include "value_objects/candidate_offset_insert.hpp"
#include "value_objects/candidate_offset_erase.hpp"

using candidate_offset_action = std::variant<candidate_offset_insert, candidate_offset_erase>;

#endif
