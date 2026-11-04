#ifndef PUD_RESUMABLE_WITNESS_SEARCH_HPP
#define PUD_RESUMABLE_WITNESS_SEARCH_HPP

#include <cstddef>
#include <optional>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_witness_advance.hpp"
#include "value_objects/pud_witness_search_stack.hpp"

// The MHWS witness search without its stored searches: a depth-first search below a root replay for a stored,
// unrefuted leaf whose replay succeeds, on a stack the caller owns. Resuming a stack continues where it stopped;
// an empty stack starts a new search.
template<typename IDescendReplay, typename IIsNodeLeaf, typename IIsNodeRefuted, typename IGetChildren>
struct pud_resumable_witness_search {
    pud_resumable_witness_search(
        IDescendReplay& descend_replay,
        IIsNodeLeaf& is_node_leaf,
        IIsNodeRefuted& is_node_refuted,
        IGetChildren& get_children);
    std::optional<pud_global_node_id> resume(const pud_replay& root, pud_witness_search_stack& search) const;
    std::optional<pud_witness_advance> advance(pud_witness_search_stack& search) const;
private:
    std::optional<pud_replay> try_descend_sibling(const pud_replay& parent, size_t& next_sibling) const;

    IDescendReplay& descend_replay_;
    IIsNodeLeaf& is_node_leaf_;
    IIsNodeRefuted& is_node_refuted_;
    IGetChildren& get_children_;
};

#define PUD_RESUMABLE_WITNESS_SEARCH_TEMPLATE                                                         \
    template<typename IDescendReplay, typename IIsNodeLeaf, typename IIsNodeRefuted, typename IGetChildren>
#define PUD_RESUMABLE_WITNESS_SEARCH                                                                  \
    pud_resumable_witness_search<IDescendReplay, IIsNodeLeaf, IIsNodeRefuted, IGetChildren>

PUD_RESUMABLE_WITNESS_SEARCH_TEMPLATE
PUD_RESUMABLE_WITNESS_SEARCH::pud_resumable_witness_search(
    IDescendReplay& descend_replay,
    IIsNodeLeaf& is_node_leaf,
    IIsNodeRefuted& is_node_refuted,
    IGetChildren& get_children)
    : descend_replay_(descend_replay)
    , is_node_leaf_(is_node_leaf)
    , is_node_refuted_(is_node_refuted)
    , get_children_(get_children) {}

PUD_RESUMABLE_WITNESS_SEARCH_TEMPLATE
std::optional<pud_global_node_id> PUD_RESUMABLE_WITNESS_SEARCH::resume(
    const pud_replay& root, pud_witness_search_stack& search) const {
    if (search.frames.empty()) {
        if (is_node_refuted_.is_refuted(root.node))
            return std::nullopt;
        if (is_node_leaf_.is_leaf(root.node))
            return root.node;
        search.frames.push_back(pud_witness_search_frame{.current = std::nullopt, .next_sibling = 0});
    }
    while (!search.frames.empty()) {
        pud_witness_search_frame& top = search.frames.back();
        if (!top.current.has_value()) {
            const size_t depth = search.frames.size();
            const pud_replay& parent = depth == 1 ? root : search.frames.at(depth - 2).current.value();
            top.current = try_descend_sibling(parent, top.next_sibling);
        }
        if (!top.current.has_value()) {
            search.frames.pop_back();
            if (!search.frames.empty())
                search.frames.back().current = std::nullopt;
            continue;
        }
        const pud_global_node_id node = top.current->node;
        if (is_node_refuted_.is_refuted(node)) {
            top.current = std::nullopt;
            continue;
        }
        if (is_node_leaf_.is_leaf(node))
            return node;
        search.frames.push_back(pud_witness_search_frame{.current = std::nullopt, .next_sibling = 0});
    }
    return std::nullopt;
}

// Moves the search's root one level down, to the node its first frame stands on, as a candidate specializes
// through it.
PUD_RESUMABLE_WITNESS_SEARCH_TEMPLATE
std::optional<pud_witness_advance> PUD_RESUMABLE_WITNESS_SEARCH::advance(pud_witness_search_stack& search) const {
    if (search.frames.empty())
        return std::nullopt;
    pud_witness_search_frame first = std::move(search.frames.front());
    search.frames.pop_front();
    return pud_witness_advance{.search_root = std::move(first.current.value()), .next_sibling = first.next_sibling};
}

PUD_RESUMABLE_WITNESS_SEARCH_TEMPLATE
std::optional<pud_replay> PUD_RESUMABLE_WITNESS_SEARCH::try_descend_sibling(
    const pud_replay& parent, size_t& next_sibling) const {
    const std::vector<pud_node_id>& siblings = get_children_.get_children(parent.node);
    while (next_sibling < siblings.size()) {
        const pud_global_node_id next{.module = parent.node.module, .node = siblings.at(next_sibling++)};
        std::optional<pud_replay> sibling = descend_replay_.descend(parent, next);
        if (sibling.has_value())
            return sibling;
    }
    return std::nullopt;
}

#undef PUD_RESUMABLE_WITNESS_SEARCH
#undef PUD_RESUMABLE_WITNESS_SEARCH_TEMPLATE

#endif
