#ifndef PUD_NODE_HPP
#define PUD_NODE_HPP

#include <vector>
#include <unordered_map>
#include "pud_specialization.hpp"
#include "expr.hpp"
#include "pud_rule_id.hpp"

struct pud_node {
    std::vector<pud_specialization> added_specializations;
    std::vector<const expr*> added_body_goals;
    uint32_t added_var_count;
    std::unordered_map<const pud_rule_id*, pud_node> children;
};

#endif
