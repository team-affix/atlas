#ifndef PUD_MHWS_HPP
#define PUD_MHWS_HPP

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "value_objects/pud_mhws_head_id.hpp"
#include "value_objects/pud_witness_advance_result.hpp"
#include "value_objects/pud_node_id.hpp"
#include "debug_assert.hpp"

template<
    typename Descent,
    typename NodeIterator,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
struct pud_mhws {
    pud_mhws(IMakeHead& make_head, IForkHead& fork_head);
    std::optional<pud_mhws_head_id> try_add_head(Descent search_root_descent);
    void remove_head(pud_mhws_head_id head_id);
    std::vector<pud_mhws_head_id> invalidate_leaf(pud_node_id node);
    std::optional<pud_witness_advance_result<Descent, NodeIterator>> advance_head(pud_mhws_head_id head_id);
    std::optional<pud_mhws_head_id> try_fork_head(pud_mhws_head_id head_id, Descent search_root_descent);
private:
    bool try_find_witness(pud_mhws_head_id head_id);
    void link(pud_mhws_head_id head_id, pud_node_id witness);
    pud_node_id unlink_head(pud_mhws_head_id head_id);
    std::unordered_set<pud_mhws_head_id> unlink_witness(pud_node_id witness);

    IMakeHead& make_head_;
    IForkHead& fork_head_;

    pud_mhws_head_id next_head_id_;
    std::unordered_map<pud_mhws_head_id, Head> heads_;
    std::unordered_map<pud_mhws_head_id, pud_node_id> head_to_witness_;
    std::unordered_map<pud_node_id, std::unordered_set<pud_mhws_head_id>> witness_to_heads_;
};

template<
    typename D,
    typename NI,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
pud_mhws<D, NI, Head, IMakeHead, IForkHead>::pud_mhws(IMakeHead& make_head, IForkHead& fork_head)
    : make_head_(make_head)
    , fork_head_(fork_head)
    , next_head_id_(0) {}

template<
    typename D,
    typename NI,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
std::optional<pud_mhws_head_id> pud_mhws<D, NI, Head, IMakeHead, IForkHead>::try_add_head(
    D search_root_descent) {
    const bool head_inserted = heads_.emplace(next_head_id_, make_head_.make(search_root_descent)).second;
    DEBUG_ASSERT(head_inserted);
    if (!try_find_witness(next_head_id_))
        return std::nullopt;
    return next_head_id_++;
}

template<
    typename D,
    typename NI,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
void pud_mhws<D, NI, Head, IMakeHead, IForkHead>::remove_head(pud_mhws_head_id head_id) {
    if (!heads_.contains(head_id))
        return;

    heads_.erase(head_id);

    unlink_head(head_id);
}

template<
    typename D,
    typename NI,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
std::vector<pud_mhws_head_id> pud_mhws<D, NI, Head, IMakeHead, IForkHead>::invalidate_leaf(pud_node_id node) {
    std::vector<pud_mhws_head_id> exhausted;
    for (pud_mhws_head_id head_id : unlink_witness(node)) {
        if (try_find_witness(head_id))
            continue;
        exhausted.push_back(head_id);
    }
    return exhausted;
}

template<
    typename D,
    typename NI,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
std::optional<pud_witness_advance_result<D, NI>> pud_mhws<D, NI, Head, IMakeHead, IForkHead>::advance_head(pud_mhws_head_id head_id) {
    auto result = heads_.at(head_id).advance();

    if (!result.has_value()) {
        heads_.erase(head_id);
        unlink_head(head_id);
    }

    return result;
}

// Resumes a head and links the witness it finds; a head that finds none is dropped.
template<
    typename D,
    typename NI,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
bool pud_mhws<D, NI, Head, IMakeHead, IForkHead>::try_find_witness(pud_mhws_head_id head_id) {
    std::optional<pud_node_id> witness = heads_.at(head_id).resume();
    if (!witness.has_value()) {
        heads_.erase(head_id);
        return false;
    }
    link(head_id, witness.value());
    return true;
}

template<
    typename D,
    typename NI,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
void pud_mhws<D, NI, Head, IMakeHead, IForkHead>::link(pud_mhws_head_id head_id, pud_node_id witness) {
    head_to_witness_.insert({head_id, witness});
    witness_to_heads_[witness].insert(head_id);
}

template<
    typename D,
    typename NI,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
pud_node_id pud_mhws<D, NI, Head, IMakeHead, IForkHead>::unlink_head(pud_mhws_head_id head_id) {
    auto extracted = head_to_witness_.extract(head_id);

    pud_node_id witness = extracted.mapped();

    auto& head_ids = witness_to_heads_.at(witness);
    head_ids.erase(head_id);

    if (head_ids.empty())
        witness_to_heads_.erase(witness);

    return witness;
}

template<
    typename D,
    typename NI,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
std::unordered_set<pud_mhws_head_id> pud_mhws<D, NI, Head, IMakeHead, IForkHead>::unlink_witness(pud_node_id witness) {
    auto extracted = witness_to_heads_.extract(witness);
    if (extracted.empty())
        return {};

    auto& head_ids = extracted.mapped();
    for (pud_mhws_head_id head_id : head_ids) {
        head_to_witness_.erase(head_id);
    }

    return std::move(head_ids);
}

template<
    typename D,
    typename NI,
    typename Head,
    typename IMakeHead,
    typename IForkHead>
std::optional<pud_mhws_head_id> pud_mhws<D, NI, Head, IMakeHead, IForkHead>::try_fork_head(pud_mhws_head_id head_id, D search_root_descent) {
    const bool head_inserted = heads_.emplace(next_head_id_, fork_head_.fork(heads_.at(head_id), search_root_descent)).second;
    DEBUG_ASSERT(head_inserted);
    if (!try_find_witness(next_head_id_))
        return std::nullopt;
    return next_head_id_++;
}

#endif
