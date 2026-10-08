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

    auto [head_it, head_inserted] = heads_.emplace(
        next_head_id_,
        make_head_.make(search_root_descent));

    DEBUG_ASSERT(head_inserted);

    Head& head = head_it->second;

    std::optional<pud_node_id> witness = head.resume();

    if (!witness.has_value()) {
        heads_.erase(head_it);
        return std::nullopt;
    }

    link(next_head_id_, witness.value());

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
    auto head_ids = unlink_witness(node);

    std::vector<pud_mhws_head_id> result;

    for (pud_mhws_head_id head_id : head_ids) {
        auto& head = heads_.at(head_id);
        std::optional<pud_node_id> new_witness = head.resume();

        if (!new_witness.has_value()) {
            heads_.erase(head_id);
            result.push_back(head_id);
            continue;
        }

        link(head_id, new_witness.value());
    }

    return std::move(result);
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
    auto& old_head = heads_.at(head_id);

    auto [new_head_it, new_head_inserted] = heads_.emplace(
        next_head_id_,
        fork_head_.fork(old_head, search_root_descent));

    DEBUG_ASSERT(new_head_inserted);

    Head& new_head = new_head_it->second;

    std::optional<pud_node_id> new_witness = new_head.resume();

    if (!new_witness.has_value()) {
        heads_.erase(new_head_it);
        return std::nullopt;
    }

    link(next_head_id_, new_witness.value());

    return next_head_id_++;
}

#endif
