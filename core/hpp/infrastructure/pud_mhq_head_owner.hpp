#ifndef PUD_MHQ_HEAD_OWNER_HPP
#define PUD_MHQ_HEAD_OWNER_HPP

#include "value_objects/pud_mhq_head_id.hpp"

// Owns one MHQ head and removes it when destroyed. Nodes share an owner through the boxes in their query maps,
// so the head goes when the last live node's map stops referencing it.
template<typename IRemoveQuery>
struct pud_mhq_head_owner {
    pud_mhq_head_owner(IRemoveQuery& remove_query, pud_mhq_head_id head_id);
    ~pud_mhq_head_owner();
    pud_mhq_head_owner(const pud_mhq_head_owner&) = delete;
    pud_mhq_head_owner& operator=(const pud_mhq_head_owner&) = delete;
    pud_mhq_head_id head_id() const;
private:
    IRemoveQuery& remove_query_;
    pud_mhq_head_id head_id_;
};

#define PUD_MHQ_HEAD_OWNER_TEMPLATE template<typename IRemoveQuery>
#define PUD_MHQ_HEAD_OWNER          pud_mhq_head_owner<IRemoveQuery>

PUD_MHQ_HEAD_OWNER_TEMPLATE
PUD_MHQ_HEAD_OWNER::pud_mhq_head_owner(IRemoveQuery& remove_query, pud_mhq_head_id head_id)
    : remove_query_(remove_query)
    , head_id_(head_id) {}

PUD_MHQ_HEAD_OWNER_TEMPLATE
PUD_MHQ_HEAD_OWNER::~pud_mhq_head_owner() {
    remove_query_.remove(head_id_);
}

PUD_MHQ_HEAD_OWNER_TEMPLATE
pud_mhq_head_id PUD_MHQ_HEAD_OWNER::head_id() const {
    return head_id_;
}

#undef PUD_MHQ_HEAD_OWNER
#undef PUD_MHQ_HEAD_OWNER_TEMPLATE

#endif
