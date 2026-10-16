#ifndef PUD_MHQ_HEAD_OWNER_HPP
#define PUD_MHQ_HEAD_OWNER_HPP

#include "value_objects/pud_mhq_head_id.hpp"

// Owns one MHQ head: removes it when the last descent sharing this owner lets go.
template<typename IRemoveQueryHead>
struct pud_mhq_head_owner {
    pud_mhq_head_owner(IRemoveQueryHead& remove_query_head, pud_mhq_head_id head_id);
    ~pud_mhq_head_owner();
    pud_mhq_head_owner(const pud_mhq_head_owner&) = delete;
    pud_mhq_head_owner& operator=(const pud_mhq_head_owner&) = delete;
    pud_mhq_head_id head_id() const;
private:
    IRemoveQueryHead& remove_query_head_;
    pud_mhq_head_id head_id_;
};

template<typename IRemoveQueryHead>
pud_mhq_head_owner<IRemoveQueryHead>::pud_mhq_head_owner(
    IRemoveQueryHead& remove_query_head,
    pud_mhq_head_id head_id)
    : remove_query_head_(remove_query_head)
    , head_id_(head_id) {}

template<typename IRemoveQueryHead>
pud_mhq_head_owner<IRemoveQueryHead>::~pud_mhq_head_owner() {
    remove_query_head_.remove_head(head_id_);
}

template<typename IRemoveQueryHead>
pud_mhq_head_id pud_mhq_head_owner<IRemoveQueryHead>::head_id() const {
    return head_id_;
}

#endif
