#ifndef PUD_INHERITED_TRAIL_SOURCE_HPP
#define PUD_INHERITED_TRAIL_SOURCE_HPP

#include <optional>
#include <immer/box.hpp>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_trails.hpp"
#include "value_objects/pud_node_trails.hpp"

// The trails a leaf's goals are queried from: those its own last visit left, else those of its nearest ancestor that
// has any, marked forked. A goal keeps its id down the path, and the leaf's bindings only add to the ancestor's, so
// the ancestor's trails resume to the leaf's candidates. A leaf with neither has none, and is queried afresh.
template<typename IGetParent, typename IFindTrails>
struct pud_inherited_trail_source {
    pud_inherited_trail_source(IGetParent& get_parent, IFindTrails& find_trails);
    pud_node_trails get(pud_global_node_id leaf) const;
private:
    IGetParent& get_parent_;
    IFindTrails& find_trails_;
};

template<typename IGetParent, typename IFindTrails>
pud_inherited_trail_source<IGetParent, IFindTrails>::pud_inherited_trail_source(
    IGetParent& get_parent, IFindTrails& find_trails)
    : get_parent_(get_parent)
    , find_trails_(find_trails) {}

template<typename IGetParent, typename IFindTrails>
pud_node_trails pud_inherited_trail_source<IGetParent, IFindTrails>::get(pud_global_node_id leaf) const {
    const pud_node_trails* own = find_trails_.find(leaf);
    if (own != nullptr)
        return *own;
    std::optional<pud_global_node_id> ancestor = get_parent_.get_parent(leaf);
    while (ancestor.has_value() && find_trails_.find(ancestor.value()) == nullptr)
        ancestor = get_parent_.get_parent(ancestor.value());
    if (!ancestor.has_value())
        return pud_node_trails{};
    const pud_node_trails ancestor_trails = *find_trails_.find(ancestor.value());
    pud_node_trails inherited = ancestor_trails;
    for (const auto& [goal, boxed] : ancestor_trails) {
        if (boxed.get().forked)
            continue;
        inherited = inherited.set(
            goal, immer::box<pud_goal_trails>{pud_goal_trails{.candidates = boxed.get().candidates, .forked = true}});
    }
    return inherited;
}

#endif
