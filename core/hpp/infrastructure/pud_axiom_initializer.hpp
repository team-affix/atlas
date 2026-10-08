#ifndef PUD_AXIOM_INITIALIZER_HPP
#define PUD_AXIOM_INITIALIZER_HPP

#include <vector>
#include "value_objects/rule.hpp"
#include "value_objects/pud_node_id.hpp"

template<typename IGetNextNodeID,
         typename IStoreHead,
         typename IStoreBodyGoals,
         typename IStoreVarCount,
         typename IRegisterRoot>
struct pud_axiom_initializer {
    pud_axiom_initializer(IGetNextNodeID& get_next_node_id,
                          IStoreHead& store_head,
                          IStoreBodyGoals& store_body_goals,
                          IStoreVarCount& store_var_count,
                          IRegisterRoot& register_root);
    pud_node_id initialize_axiom(const rule& axiom);
private:
    IGetNextNodeID& get_next_node_id_;
    IStoreHead& store_head_;
    IStoreBodyGoals& store_body_goals_;
    IStoreVarCount& store_var_count_;
    IRegisterRoot& register_root_;
};

template<typename IGNID, typename ISH, typename ISBG, typename ISVC, typename IRR>
pud_axiom_initializer<IGNID, ISH, ISBG, ISVC, IRR>::pud_axiom_initializer(
    IGNID& get_next_node_id,
    ISH& store_head,
    ISBG& store_body_goals,
    ISVC& store_var_count,
    IRR& register_root)
    : get_next_node_id_(get_next_node_id)
    , store_head_(store_head)
    , store_body_goals_(store_body_goals)
    , store_var_count_(store_var_count)
    , register_root_(register_root)
{}

template<typename IGNID, typename ISH, typename ISBG, typename ISVC, typename IRR>
pud_node_id pud_axiom_initializer<IGNID, ISH, ISBG, ISVC, IRR>::initialize_axiom(const rule& axiom) {
    const pud_node_id id = get_next_node_id_.next();
    store_head_.store(id, axiom.head);
    store_body_goals_.store(id, std::vector<const expr*>(axiom.body.begin(), axiom.body.end()));
    store_var_count_.store(id, axiom.var_count);
    register_root_.register_root(id);
    return id;
}

#endif
