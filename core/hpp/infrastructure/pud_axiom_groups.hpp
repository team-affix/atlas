#ifndef PUD_AXIOM_GROUPS_HPP
#define PUD_AXIOM_GROUPS_HPP

#include <cstddef>
#include <vector>
#include "value_objects/pud_axiom_group_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Partitions axiom roots into groups. A query opened in group G may use candidate roots from G and
// every group G depends on, never from a group that depends on G. A query sees only the roots its
// group could see when it was opened, so once a group's roots are read, no group it sees may gain
// members or dependencies.
struct pud_axiom_groups {
    pud_axiom_groups();
    pud_axiom_group_id create_group();
    void add_dependency(pud_axiom_group_id dependent, pud_axiom_group_id dependency);
    void add_member(pud_axiom_group_id group, pud_node_id root);
    const std::vector<pud_node_id>& axiom_roots(pud_axiom_group_id group) const;
private:
    void visit_dependencies(
        pud_axiom_group_id group,
        std::vector<bool>& visited,
        std::vector<pud_axiom_group_id>& dependencies_first) const;

    size_t next_group_id_;
    std::vector<std::vector<pud_axiom_group_id>> dependencies_;
    std::vector<std::vector<pud_node_id>> members_;
    mutable std::vector<std::vector<pud_node_id>> visible_roots_;
    mutable std::vector<bool> visible_roots_ready_;
    mutable std::vector<bool> sealed_;
};

#endif
