#ifndef PUD_AXIOM_GROUPS_HPP
#define PUD_AXIOM_GROUPS_HPP

#include <cstddef>
#include <vector>
#include "infrastructure/pud_node_table.hpp"
#include "value_objects/pud_axiom_group_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Partitions rule roots into groups. A leaf in group G may unfold using candidate roots from G and
// every group G depends on, never from a group that depends on G.
struct pud_axiom_groups {
    pud_axiom_groups();
    pud_axiom_group_id create_group();
    void add_dependency(pud_axiom_group_id dependent, pud_axiom_group_id dependency);
    void add_member(pud_axiom_group_id group, pud_node_id root);
    void store_node_group(pud_node_id node, pud_axiom_group_id group);
    pud_axiom_group_id node_group(pud_node_id node) const;
    const std::vector<pud_node_id>& axiom_roots(pud_node_id query_leaf) const;
private:
    void invalidate_visible_roots_cache();
    const std::vector<pud_node_id>& visible_roots_for(pud_axiom_group_id group) const;

    size_t next_group_id_;
    std::vector<std::vector<pud_axiom_group_id>> dependencies_;
    std::vector<std::vector<pud_node_id>> members_;
    pud_node_table<pud_axiom_group_id> node_groups_;
    mutable std::vector<std::vector<pud_node_id>> visible_roots_cache_;
    mutable std::vector<bool> visible_roots_ready_;
};

#endif
