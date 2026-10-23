#ifndef PUD_MHWS_HPP
#define PUD_MHWS_HPP

#include <cstddef>
#include <deque>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "value_objects/pud_mhws_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_witness_advance.hpp"
#include "debug_assert.hpp"

// The multi-headed witness search. Each head searches the subtree under its search root depth first,
// children in order, for a live leaf its query reaches: its witness. A head whose witness stops being
// a live leaf resumes from where it stopped; a head that runs out of subtree is exhausted and dropped.
template<typename IDescendReplay, typename ICheckNodeLeaf, typename ICheckNodeRefuted, typename IGetChildren>
struct pud_mhws {
    pud_mhws(
        IDescendReplay& descend_replay,
        ICheckNodeLeaf& check_node_leaf,
        ICheckNodeRefuted& check_node_refuted,
        IGetChildren& get_children);
    std::optional<pud_mhws_head_id> try_add(pud_replay search_root);
    std::optional<pud_mhws_head_id> try_fork(pud_mhws_head_id head_id, pud_replay search_root);
    std::optional<pud_witness_advance> advance(pud_mhws_head_id head_id);
    void remove(pud_mhws_head_id head_id);
    std::vector<pud_mhws_head_id> invalidate_leaf(pud_node_id leaf);
private:
    // One level below the search root: the node the search stands on, none while it moves on to the
    // next sibling, and the position of that next sibling among the parent's children.
    struct frame {
        std::optional<pud_replay> current;
        size_t next_sibling;
    };
    struct search {
        pud_replay root;
        std::deque<frame> frames;
    };

    std::optional<pud_mhws_head_id> try_admit(search admitted);
    std::optional<pud_node_id> resume(search& resumed);
    std::optional<pud_replay> try_descend_sibling(const pud_replay& parent, size_t& next_sibling);
    void link(pud_mhws_head_id head_id, pud_node_id witness);
    void unlink(pud_mhws_head_id head_id);

    IDescendReplay& descend_replay_;
    ICheckNodeLeaf& check_node_leaf_;
    ICheckNodeRefuted& check_node_refuted_;
    IGetChildren& get_children_;

    pud_mhws_head_id next_head_id_;
    std::unordered_map<pud_mhws_head_id, search> searches_;
    std::unordered_map<pud_mhws_head_id, pud_node_id> head_to_witness_;
    std::unordered_map<pud_node_id, std::unordered_set<pud_mhws_head_id>> witness_to_heads_;
};

#define PUD_MHWS_TEMPLATE                                                                  \
    template<typename IDescendReplay, typename ICheckNodeLeaf, typename ICheckNodeRefuted, \
             typename IGetChildren>
#define PUD_MHWS pud_mhws<IDescendReplay, ICheckNodeLeaf, ICheckNodeRefuted, IGetChildren>

PUD_MHWS_TEMPLATE
PUD_MHWS::pud_mhws(
    IDescendReplay& descend_replay,
    ICheckNodeLeaf& check_node_leaf,
    ICheckNodeRefuted& check_node_refuted,
    IGetChildren& get_children)
    : descend_replay_(descend_replay)
    , check_node_leaf_(check_node_leaf)
    , check_node_refuted_(check_node_refuted)
    , get_children_(get_children)
    , next_head_id_(0)
    , searches_()
    , head_to_witness_()
    , witness_to_heads_() {}

PUD_MHWS_TEMPLATE
std::optional<pud_mhws_head_id> PUD_MHWS::try_add(pud_replay search_root) {
    return try_admit(search{.root = std::move(search_root), .frames = {}});
}

// Replays the other head's path from the new search root. Where the path no longer replays, the
// search stops there and moves on to that frame's later siblings.
PUD_MHWS_TEMPLATE
std::optional<pud_mhws_head_id> PUD_MHWS::try_fork(pud_mhws_head_id head_id, pud_replay search_root) {
    search forked{.root = std::move(search_root), .frames = {}};
    for (const frame& other_frame : searches_.at(head_id).frames) {
        const pud_replay& parent = forked.frames.empty() ? forked.root : forked.frames.back().current.value();
        std::optional<pud_replay> replayed = descend_replay_.descend(parent, other_frame.current.value().node);
        const bool stopped = !replayed.has_value();
        forked.frames.push_back(frame{.current = std::move(replayed), .next_sibling = other_frame.next_sibling});
        if (stopped)
            break;
    }
    return try_admit(std::move(forked));
}

// A head whose witness is its own search root has nothing left below it and is dropped.
PUD_MHWS_TEMPLATE
std::optional<pud_witness_advance> PUD_MHWS::advance(pud_mhws_head_id head_id) {
    search& advanced = searches_.at(head_id);
    if (advanced.frames.empty()) {
        remove(head_id);
        return std::nullopt;
    }
    frame first = std::move(advanced.frames.front());
    advanced.frames.pop_front();
    advanced.root = first.current.value();
    return pud_witness_advance{.search_root = std::move(first.current.value()), .next_sibling = first.next_sibling};
}

PUD_MHWS_TEMPLATE
void PUD_MHWS::remove(pud_mhws_head_id head_id) {
    unlink(head_id);
    searches_.erase(head_id);
}

PUD_MHWS_TEMPLATE
std::vector<pud_mhws_head_id> PUD_MHWS::invalidate_leaf(pud_node_id leaf) {
    std::vector<pud_mhws_head_id> unwitnessed;
    auto holders = witness_to_heads_.extract(leaf);
    if (!holders.empty())
        unwitnessed.insert(unwitnessed.end(), holders.mapped().begin(), holders.mapped().end());
    std::vector<pud_mhws_head_id> exhausted;
    for (pud_mhws_head_id head_id : unwitnessed) {
        head_to_witness_.erase(head_id);
        const std::optional<pud_node_id> witness = resume(searches_.at(head_id));
        if (witness.has_value()) {
            link(head_id, witness.value());
            continue;
        }
        searches_.erase(head_id);
        exhausted.push_back(head_id);
    }
    return exhausted;
}

PUD_MHWS_TEMPLATE
std::optional<pud_mhws_head_id> PUD_MHWS::try_admit(search admitted) {
    const std::optional<pud_node_id> witness = resume(admitted);
    if (!witness.has_value())
        return std::nullopt;
    const pud_mhws_head_id head_id = next_head_id_++;
    searches_.emplace(head_id, std::move(admitted));
    link(head_id, witness.value());
    return head_id;
}

PUD_MHWS_TEMPLATE
std::optional<pud_node_id> PUD_MHWS::resume(search& resumed) {
    if (resumed.frames.empty()) {
        const pud_node_id root = resumed.root.node;
        if (check_node_refuted_.check_refuted(root))
            return std::nullopt;
        if (check_node_leaf_.check_leaf(root))
            return root;
        resumed.frames.push_back(frame{.current = std::nullopt, .next_sibling = 0});
    }
    while (!resumed.frames.empty()) {
        frame& top = resumed.frames.back();
        if (!top.current.has_value()) {
            const size_t depth = resumed.frames.size();
            const pud_replay& parent = depth == 1 ? resumed.root : resumed.frames.at(depth - 2).current.value();
            top.current = try_descend_sibling(parent, top.next_sibling);
        }
        if (!top.current.has_value()) {
            resumed.frames.pop_back();
            if (!resumed.frames.empty())
                resumed.frames.back().current = std::nullopt;
            continue;
        }
        const pud_node_id node = top.current->node;
        if (check_node_refuted_.check_refuted(node)) {
            top.current = std::nullopt;
            continue;
        }
        if (check_node_leaf_.check_leaf(node))
            return node;
        resumed.frames.push_back(frame{.current = std::nullopt, .next_sibling = 0});
    }
    return std::nullopt;
}

PUD_MHWS_TEMPLATE
std::optional<pud_replay> PUD_MHWS::try_descend_sibling(const pud_replay& parent, size_t& next_sibling) {
    const std::vector<pud_node_id>& siblings = get_children_.get(parent.node);
    while (next_sibling < siblings.size()) {
        std::optional<pud_replay> sibling = descend_replay_.descend(parent, siblings.at(next_sibling++));
        if (sibling.has_value())
            return sibling;
    }
    return std::nullopt;
}

PUD_MHWS_TEMPLATE
void PUD_MHWS::link(pud_mhws_head_id head_id, pud_node_id witness) {
    head_to_witness_.emplace(head_id, witness);
    witness_to_heads_[witness].insert(head_id);
}

PUD_MHWS_TEMPLATE
void PUD_MHWS::unlink(pud_mhws_head_id head_id) {
    auto witness = head_to_witness_.extract(head_id);
    std::unordered_set<pud_mhws_head_id>& heads = witness_to_heads_.at(witness.mapped());
    heads.erase(head_id);
    if (heads.empty())
        witness_to_heads_.erase(witness.mapped());
}

#undef PUD_MHWS
#undef PUD_MHWS_TEMPLATE

#endif
