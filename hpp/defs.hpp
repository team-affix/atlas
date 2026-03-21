#ifndef DEFS_HPP
#define DEFS_HPP

#include <list>
#include <map>
#include "lineage.hpp"
#include "expr.hpp"
#include "rule.hpp"

using goals = std::list<const expr*>;
using goal_store = std::map<const goal_lineage*, const expr*>;
using candidate_store = std::multimap<const goal_lineage*, size_t>;
using resolution_store = std::set<const resolution_lineage*>;
using decision_store = std::set<const resolution_lineage*>;
using avoidance = std::set<const resolution_lineage*>;
using avoidance_store = std::set<avoidance>;
using avoidance_map = std::multimap<const goal_lineage*, const avoidance>;
using database = std::vector<rule>;

#endif
