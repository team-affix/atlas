#ifndef IMPORT_GOALS_FROM_STRING_HPP
#define IMPORT_GOALS_FROM_STRING_HPP

#include <string>
#include "../../core/hpp/defs.hpp"
#include "../../core/hpp/expr.hpp"
#include "../../core/hpp/sequencer.hpp"

goals import_goals_from_string(const std::string& body, expr_pool&, sequencer&);

#endif
