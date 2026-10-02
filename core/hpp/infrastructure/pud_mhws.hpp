#ifndef PUD_MHWS_HPP
#define PUD_MHWS_HPP

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "value_objects/pud_mhws_head_id.hpp"
#include "value_objects/pud_query_frame.hpp"
#include "debug_assert.hpp"

template<
    typename QueryHandle,
    typename ChildIterator,
    typename IMakeHead,
    typename IForkHead>
struct pud_mhws {
    pud_mhws(IMakeHead& make_head, IForkHead& fork_head);
    std::optional<pud_mhws_head_id> try_add_head(pud_query_position<QueryHandle> search_root_position);
    void remove_head(pud_mhws_head_id head_id);
    std::vector<pud_mhws_head_id> invalidate_leaf(const pud_node* node);
    pud_query_frame<QueryHandle, ChildIterator> advance_head(pud_mhws_head_id head_id);
    std::optional<pud_mhws_head_id> try_fork_head(pud_mhws_head_id head_id, QueryHandle new_query_handle);
private:
    using head_type = decltype(std::declval<IMakeHead&>().make(
        std::declval<pud_query_position<QueryHandle>>()));

    void link(pud_mhws_head_id head_id, const pud_node* witness);
    const pud_node* unlink_head(pud_mhws_head_id head_id);
    std::unordered_set<pud_mhws_head_id> unlink_witness(const pud_node* witness);

    IMakeHead& make_head_;
    IForkHead& fork_head_;

    pud_mhws_head_id next_head_id_;
    std::unordered_map<pud_mhws_head_id, head_type> heads_;
    std::unordered_map<pud_mhws_head_id, const pud_node*> head_to_witness_;
    std::unordered_map<const pud_node*, std::unordered_set<pud_mhws_head_id>> witness_to_heads_;
};

template<
    typename QH,
    typename CI,
    typename IMakeHead,
    typename IForkHead>
pud_mhws<QH, CI, IMakeHead, IForkHead>::pud_mhws(IMakeHead& make_head, IForkHead& fork_head)
    : make_head_(make_head)
    , fork_head_(fork_head)
    , next_head_id_(0) {}

template<
    typename QH,
    typename CI,
    typename IMakeHead,
    typename IForkHead>
std::optional<pud_mhws_head_id> pud_mhws<QH, CI, IMakeHead, IForkHead>::try_add_head(
    pud_query_position<QH> search_root_position) {
    auto [head_it, head_inserted] = heads_.emplace(
        next_head_id_,
        make_head_.make(std::move(search_root_position)));

    DEBUG_ASSERT(head_inserted);

    head_type& head = head_it->second;

    std::optional<const pud_node*> witness = head.resume();

    if (!witness.has_value()) {
        heads_.erase(head_it);
        return std::nullopt;
    }
    
    link(next_head_id_, witness.value());
    
    return next_head_id_++;
}

template<
    typename QH,
    typename CI,
    typename IMakeHead,
    typename IForkHead>
void pud_mhws<QH, CI, IMakeHead, IForkHead>::remove_head(pud_mhws_head_id head_id) {
    if (!heads_.contains(head_id))
        return;

    heads_.erase(head_id);
    
    unlink_head(head_id);
}

template<
    typename QH,
    typename CI,
    typename IMakeHead,
    typename IForkHead>
std::vector<pud_mhws_head_id> pud_mhws<QH, CI, IMakeHead, IForkHead>::invalidate_leaf(const pud_node* node) {
    auto head_ids = unlink_witness(node);

    std::vector<pud_mhws_head_id> result;

    for (pud_mhws_head_id head_id : head_ids) {
        auto& head = heads_.at(head_id);
        const pud_node* new_witness = head.resume();

        if (new_witness != nullptr) {
            link(head_id, new_witness);
            continue;
        }
        
        heads_.erase(head_id);
        result.push_back(head_id);
    }

    return std::move(result);
}

template<
    typename QH,
    typename CI,
    typename IMakeHead,
    typename IForkHead>
pud_query_frame<QH, CI> pud_mhws<QH, CI, IMakeHead, IForkHead>::advance_head(pud_mhws_head_id head_id) {
    auto& head = heads_.at(head_id);
    auto [root_frame, dead_head] = head.advance_root();

    if (dead_head) {
        heads_.erase(head_id);
        unlink_head(head_id);
    }

    return root_frame;
}

template<
    typename QH,
    typename CI,
    typename IMakeHead,
    typename IForkHead>
void pud_mhws<QH, CI, IMakeHead, IForkHead>::link(pud_mhws_head_id head_id, const pud_node* witness) {
    head_to_witness_.insert({next_head_id_, witness});
    witness_to_heads_[witness].insert(next_head_id_);
}

template<
    typename QH,
    typename CI,
    typename IMakeHead,
    typename IForkHead>
const pud_node* pud_mhws<QH, CI, IMakeHead, IForkHead>::unlink_head(pud_mhws_head_id head_id) {
    auto extracted = head_to_witness_.extract(head_id);

    const pud_node* witness = extracted.mapped();

    auto& head_ids = witness_to_heads_.at(witness);
    head_ids.erase(head_id);

    if (head_ids.empty())
        witness_to_heads_.erase(witness);

    return witness;
}

template<
    typename QH,
    typename CI,
    typename IMakeHead,
    typename IForkHead>
std::unordered_set<pud_mhws_head_id> pud_mhws<QH, CI, IMakeHead, IForkHead>::unlink_witness(const pud_node* witness) {
    auto extracted = witness_to_heads_.extract(witness);

    for (pud_mhws_head_id head_id : extracted.mapped()) {
        head_to_witness_.erase(head_id);
    }

    return std::move(extracted.mapped());
}

template<
    typename QH,
    typename CI,
    typename IMakeHead,
    typename IForkHead>
std::optional<pud_mhws_head_id> pud_mhws<QH, CI, IMakeHead, IForkHead>::try_fork_head(pud_mhws_head_id head_id, QH new_query_handle) {
    auto& old_head = heads_.at(head_id);

    auto [new_head_it, new_head_inserted] = heads_.emplace(
        next_head_id_,
        fork_head_.fork(old_head, new_query_handle));

    DEBUG_ASSERT(new_head_inserted);

    head_type& new_head = new_head_it->second;

    std::optional<const pud_node*> new_witness = new_head.resume();

    if (!new_witness.has_value()) {
        heads_.erase(new_head_it);
        return std::nullopt;
    }
    
    link(next_head_id_, new_witness.value());

    return next_head_id_++;
}

#endif
