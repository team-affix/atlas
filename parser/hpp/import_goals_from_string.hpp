#ifndef IMPORT_GOALS_FROM_STRING_HPP
#define IMPORT_GOALS_FROM_STRING_HPP

#include <map>
#include <string>
#include <cstdint>
#include "../../core/hpp/defs.hpp"
#include "../../core/hpp/expr.hpp"
#include "../../core/hpp/sequencer.hpp"

struct parsed_goals {
    goals gl;
    std::map<std::string, uint32_t> var_map;
};

parsed_goals import_goals_from_string(const std::string& body, expr_pool&, sequencer&);

#endif
