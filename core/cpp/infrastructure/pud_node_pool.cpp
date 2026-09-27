#include "infrastructure/pud_node_pool.hpp"

const pud_node* pud_node_pool::make(
    std::vector<pud_specialization> added_specializations,
    std::vector<const expr*> added_body_goals,
    uint32_t added_var_count) {
    return intern(pud_node{
        .added_specializations = std::move(added_specializations),
        .added_body_goals = std::move(added_body_goals),
        .added_var_count = added_var_count
    });
}

const pud_node* pud_node_pool::intern(pud_node&& node) {
    return &nodes_.emplace_back(std::move(node));
}
