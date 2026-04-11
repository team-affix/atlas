#ifndef DEFS_HPP
#define DEFS_HPP

#include <vector>
#include "lineage.hpp"
#include "expr.hpp"
#include "rule.hpp"

using goals = std::vector<const expr*>;
using resolutions = std::set<const resolution_lineage*>;
using database = std::vector<rule>;

#endif
