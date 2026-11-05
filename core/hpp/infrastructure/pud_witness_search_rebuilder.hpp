#ifndef PUD_WITNESS_SEARCH_REBUILDER_HPP
#define PUD_WITNESS_SEARCH_REBUILDER_HPP

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_witness_search_stack.hpp"
#include "debug_assert.hpp"

// Rebuilds the stack of a witness search that stood on the last node of a chain below its root, replaying down the
// chain. Each frame continues after its node among its parent's children. A node that no longer replays leaves its
// frame without a node and ends the stack there, so resuming moves on to that node's next sibling.
template<typename IDescendReplay, typename IGetChildren>
struct pud_witness_search_rebuilder {
    pud_witness_search_rebuilder(IDescendReplay& descend_replay, IGetChildren& get_children);
    pud_witness_search_stack rebuild(
        const pud_replay& search_root, const std::vector<pud_global_node_id>& chain) const;
private:
    IDescendReplay& descend_replay_;
    IGetChildren& get_children_;
};

template<typename IDescendReplay, typename IGetChildren>
pud_witness_search_rebuilder<IDescendReplay, IGetChildren>::pud_witness_search_rebuilder(
    IDescendReplay& descend_replay, IGetChildren& get_children)
    : descend_replay_(descend_replay)
    , get_children_(get_children) {}

template<typename IDescendReplay, typename IGetChildren>
pud_witness_search_stack pud_witness_search_rebuilder<IDescendReplay, IGetChildren>::rebuild(
    const pud_replay& search_root, const std::vector<pud_global_node_id>& chain) const {
    pud_witness_search_stack search{.frames = {}};
    const pud_replay* parent = &search_root;
    for (const pud_global_node_id node : chain) {
        const std::vector<pud_node_id>& siblings = get_children_.get_children(parent->node);
        const auto position = std::find(siblings.begin(), siblings.end(), node.node);
        DEBUG_ASSERT(position != siblings.end());
        const size_t next_sibling = static_cast<size_t>(position - siblings.begin()) + 1;
        search.frames.push_back(
            pud_witness_search_frame{.current = descend_replay_.descend(*parent, node), .next_sibling = next_sibling});
        if (!search.frames.back().current.has_value())
            break;
        parent = &search.frames.back().current.value();
    }
    return search;
}

#endif
