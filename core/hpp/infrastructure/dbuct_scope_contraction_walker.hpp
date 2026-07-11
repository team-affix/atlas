#ifndef DBUCT_SCOPE_CONTRACTION_WALKER_HPP
#define DBUCT_SCOPE_CONTRACTION_WALKER_HPP

#include <map>
#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_node_id.hpp"

template<typename IMakeResolutionLineage>
struct dbuct_scope_contraction_walker {
    static constexpr bool use_unordered_tables = false;

    dbuct_scope_contraction_walker(IMakeResolutionLineage& make_resolution_lineage);

    mcts_node_id walk(const mcts_node_id& node, const mcts_choice& choice) const;

    static mcts_node_id make_root();

private:
    IMakeResolutionLineage& make_resolution_lineage_;
};

template<typename IMakeResolutionLineage>
dbuct_scope_contraction_walker<IMakeResolutionLineage>::dbuct_scope_contraction_walker(
    IMakeResolutionLineage& make_resolution_lineage)
    : make_resolution_lineage_(make_resolution_lineage) {}

template<typename IMakeResolutionLineage>
mcts_node_id dbuct_scope_contraction_walker<IMakeResolutionLineage>::walk(
        const mcts_node_id& node, const mcts_choice& choice) const {
    if (const goal_lineage* const* goal =
            std::get_if<const goal_lineage*>(&choice)) {
        return {node.first, *goal};
    }
    const resolution_lineage* resolution =
        make_resolution_lineage_.make_resolution_lineage(
            node.second, std::get<rule_id>(choice));
    mcts_node_id::first_type next_set = node.first;
    next_set.insert(resolution);
    return {std::move(next_set), nullptr};
}

template<typename IMakeResolutionLineage>
mcts_node_id dbuct_scope_contraction_walker<IMakeResolutionLineage>::make_root() {
    return {mcts_node_id::first_type{}, nullptr};
}

#endif
