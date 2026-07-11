#ifndef DBUCT_SET_WALKER_HPP
#define DBUCT_SET_WALKER_HPP

#include <map>
#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_node_id.hpp"

template<typename IMakeResolutionLineage>
struct dbuct_set_walker {
    using node_id_t = mcts_node_id;

    static constexpr bool use_unordered_tables = false;

    dbuct_set_walker(IMakeResolutionLineage& make_resolution_lineage);

    node_id_t walk(const node_id_t& node, const mcts_choice& choice) const;

    static node_id_t make_root();

private:
    IMakeResolutionLineage& make_resolution_lineage_;
};

template<typename IMakeResolutionLineage>
dbuct_set_walker<IMakeResolutionLineage>::dbuct_set_walker(
    IMakeResolutionLineage& make_resolution_lineage)
    : make_resolution_lineage_(make_resolution_lineage) {}

template<typename IMakeResolutionLineage>
mcts_node_id dbuct_set_walker<IMakeResolutionLineage>::walk(
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
mcts_node_id dbuct_set_walker<IMakeResolutionLineage>::make_root() {
    return {mcts_node_id::first_type{}, nullptr};
}

#endif
