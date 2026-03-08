#ifndef A01_DEFS_HPP
#define A01_DEFS_HPP

#include "lineage.hpp"
#include "expr.hpp"
#include "rule.hpp"

using a01_goals = std::set<const expr*>;
using a01_goal_store = std::map<const goal_lineage*, const expr*>;
using a01_candidate_store = std::multimap<const goal_lineage*, size_t>;
using a01_resolution_store = std::set<const resolution_lineage*>;
using a01_decision_store = std::set<const resolution_lineage*>;
using a01_avoidance_store = std::set<a01_decision_store>;
using a01_database = std::vector<rule>;

#endif
