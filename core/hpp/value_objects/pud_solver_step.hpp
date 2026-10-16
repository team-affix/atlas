#ifndef PUD_SOLVER_STEP_HPP
#define PUD_SOLVER_STEP_HPP

#include <variant>
#include "value_objects/pud_decision.hpp"
#include "value_objects/pud_solution.hpp"

using pud_solver_step = std::variant<pud_decision, pud_solution>;

#endif
