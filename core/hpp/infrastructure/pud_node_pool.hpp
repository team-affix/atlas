#ifndef PUD_NODE_POOL_HPP
#define PUD_NODE_POOL_HPP

#include <deque>
#include "value_objects/pud_node.hpp"

struct pud_node_pool {
    const pud_node* make(
        std::vector<pud_specialization> added_specializations,
        std::vector<const expr*> added_body_goals,
        uint32_t added_var_count);
private:
    const pud_node* intern(pud_node&& node);
    std::deque<pud_node> nodes_;
};

#endif
