#ifndef DEFS_HPP
#define DEFS_HPP

#include <list>
#include <vector>
#include <unordered_set>
#include "lineage.hpp"
#include "expr.hpp"
#include "rule.hpp"

using goals = std::vector<expr_id>;
using resolutions = std::unordered_set<const resolution_lineage*>;
using decisions = std::unordered_set<const resolution_lineage*>;
using database = std::vector<rule>;

#endif
