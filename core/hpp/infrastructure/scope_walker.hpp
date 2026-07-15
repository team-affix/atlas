#ifndef SCOPE_WALKER_HPP
#define SCOPE_WALKER_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_scope_node_id.hpp"

template<typename IMakeResolutionLineage>
struct scope_walker {
    scope_walker(IMakeResolutionLineage& make_resolution_lineage);

    mcts_scope_node_id walk(const mcts_scope_node_id& node, const mcts_choice& choice) const;

private:
    IMakeResolutionLineage& make_resolution_lineage_;
};

template<typename IMakeResolutionLineage>
scope_walker<IMakeResolutionLineage>::scope_walker(
    IMakeResolutionLineage& make_resolution_lineage)
    : make_resolution_lineage_(make_resolution_lineage) {}

template<typename IMakeResolutionLineage>
mcts_scope_node_id scope_walker<IMakeResolutionLineage>::walk(
        const mcts_scope_node_id& node, const mcts_choice& choice) const {
    if (const goal_lineage* const* goal =
            std::get_if<const goal_lineage*>(&choice)) {
        return {node.first, *goal};
    }
    const resolution_lineage* resolution =
        make_resolution_lineage_.make_resolution_lineage(
            node.second, std::get<rule_id>(choice));
    mcts_scope_node_id::first_type next_set = node.first;
    next_set.insert(resolution);
    return {std::move(next_set), nullptr};
}

#endif
