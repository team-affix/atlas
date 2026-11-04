#ifndef PUD_QUERY_REMOVER_HPP
#define PUD_QUERY_REMOVER_HPP

#include "value_objects/pud_mhq_head_id.hpp"

// Retires a query once no live node shares its owner: forgets where it was introduced, then removes its head.
template<typename IWithdrawIntroduction, typename IRemoveQuery>
struct pud_query_remover {
    pud_query_remover(IWithdrawIntroduction& withdraw_introduction, IRemoveQuery& remove_query);
    void remove(pud_mhq_head_id head_id);
private:
    IWithdrawIntroduction& withdraw_introduction_;
    IRemoveQuery& remove_query_;
};

#define PUD_QUERY_REMOVER_TEMPLATE template<typename IWithdrawIntroduction, typename IRemoveQuery>
#define PUD_QUERY_REMOVER          pud_query_remover<IWithdrawIntroduction, IRemoveQuery>

PUD_QUERY_REMOVER_TEMPLATE
PUD_QUERY_REMOVER::pud_query_remover(IWithdrawIntroduction& withdraw_introduction, IRemoveQuery& remove_query)
    : withdraw_introduction_(withdraw_introduction)
    , remove_query_(remove_query) {}

PUD_QUERY_REMOVER_TEMPLATE
void PUD_QUERY_REMOVER::remove(pud_mhq_head_id head_id) {
    withdraw_introduction_.withdraw(head_id);
    remove_query_.remove(head_id);
}

#undef PUD_QUERY_REMOVER
#undef PUD_QUERY_REMOVER_TEMPLATE

#endif
