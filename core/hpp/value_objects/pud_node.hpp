#ifndef PUD_NODE_HPP
#define PUD_NODE_HPP

#include <vector>
#include "pud_specialization.hpp"
#include "expr.hpp"

struct pud_node {
    std::vector<pud_specialization> added_specializations;
    std::vector<const expr*> added_body_goals;
    uint32_t added_var_count;
};

#endif
