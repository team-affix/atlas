#ifndef IMPORT_GOALS_FROM_STRING_HPP
#define IMPORT_GOALS_FROM_STRING_HPP

#include <cstdint>
#include <map>
#include <string>
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"
#include "interfaces/i_var_sequencer.hpp"
#include "interfaces/i_push_initial_goal_expr.hpp"

std::map<std::string, uint32_t> import_goals_from_string(
    const std::string& body,
    i_make_functor&,
    i_make_var&,
    i_var_sequencer&,
    i_push_initial_goal_expr&,
    std::map<std::string, uint32_t>& atom_map,
    uint32_t& next_atom_id);

#endif
