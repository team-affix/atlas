#include "infrastructure/pud_axiom_groups.hpp"
#include <algorithm>
#include "debug_assert.hpp"

pud_axiom_groups::pud_axiom_groups()
    : next_group_id_(0)
    , dependencies_()
    , members_()
    , visible_roots_()
    , visible_roots_ready_()
    , sealed_() {}

pud_axiom_group_id pud_axiom_groups::create_group() {
    const pud_axiom_group_id group{.value = next_group_id_++};
    dependencies_.emplace_back();
    members_.emplace_back();
    visible_roots_.emplace_back();
    visible_roots_ready_.push_back(false);
    sealed_.push_back(false);
    return group;
}

void pud_axiom_groups::add_dependency(pud_axiom_group_id dependent, pud_axiom_group_id dependency) {
    DEBUG_ASSERT(!sealed_.at(dependent.value));
    std::vector<pud_axiom_group_id>& dependencies = dependencies_.at(dependent.value);
    if (std::ranges::find(dependencies, dependency) != dependencies.end())
        return;
    std::vector<bool> reachable(next_group_id_, false);
    std::vector<pud_axiom_group_id> dependencies_first;
    visit_dependencies(dependency, reachable, dependencies_first);
    const bool cyclic = reachable.at(dependent.value);
    DEBUG_ASSERT(!cyclic);
    if (cyclic)
        return;
    dependencies.push_back(dependency);
}

void pud_axiom_groups::add_member(pud_axiom_group_id group, pud_node_id root) {
    DEBUG_ASSERT(!sealed_.at(group.value));
    members_.at(group.value).push_back(root);
}

// Every group the group sees, dependencies before their dependents, then members in the order added.
const std::vector<pud_node_id>& pud_axiom_groups::axiom_roots(pud_axiom_group_id group) const {
    std::vector<pud_node_id>& visible = visible_roots_.at(group.value);
    if (visible_roots_ready_.at(group.value))
        return visible;
    std::vector<bool> visited(next_group_id_, false);
    std::vector<pud_axiom_group_id> dependencies_first;
    visit_dependencies(group, visited, dependencies_first);
    for (pud_axiom_group_id seen : dependencies_first) {
        sealed_.at(seen.value) = true;
        const std::vector<pud_node_id>& seen_members = members_.at(seen.value);
        visible.insert(visible.end(), seen_members.begin(), seen_members.end());
    }
    visible_roots_ready_.at(group.value) = true;
    return visible;
}

void pud_axiom_groups::visit_dependencies(
    pud_axiom_group_id group,
    std::vector<bool>& visited,
    std::vector<pud_axiom_group_id>& dependencies_first) const {
    if (visited.at(group.value))
        return;
    visited.at(group.value) = true;
    for (pud_axiom_group_id dependency : dependencies_.at(group.value))
        visit_dependencies(dependency, visited, dependencies_first);
    dependencies_first.push_back(group);
}
