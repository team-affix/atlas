#ifndef PUD_CONJECTURE_INITIALIZER_HPP
#define PUD_CONJECTURE_INITIALIZER_HPP

#include <cstdint>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_node_id.hpp"

// Starts a conjecture's tree: a headless root that is not an axiom, so nothing unfolds onto it.
// Its root descent is made and activated right away, so the conjecture joins the open leaves.
template<typename IGetNextNodeID,
         typename IStoreBodyGoals,
         typename IStoreVarCount,
         typename IMarkRoot,
         typename ISetLeaf,
         typename IMakeRootDescent,
         typename IActivateLeaf>
struct pud_conjecture_initializer {
    pud_conjecture_initializer(IGetNextNodeID& get_next_node_id,
                               IStoreBodyGoals& store_body_goals,
                               IStoreVarCount& store_var_count,
                               IMarkRoot& mark_root,
                               ISetLeaf& set_leaf,
                               IMakeRootDescent& make_root_descent,
                               IActivateLeaf& activate_leaf);
    pud_node_id initialize_conjecture(const std::vector<const expr*>& goals, uint32_t var_count);
private:
    IGetNextNodeID& get_next_node_id_;
    IStoreBodyGoals& store_body_goals_;
    IStoreVarCount& store_var_count_;
    IMarkRoot& mark_root_;
    ISetLeaf& set_leaf_;
    IMakeRootDescent& make_root_descent_;
    IActivateLeaf& activate_leaf_;
};

template<typename IGNID, typename ISBG, typename ISVC, typename IMR, typename ISL, typename IMRD, typename IAL>
pud_conjecture_initializer<IGNID, ISBG, ISVC, IMR, ISL, IMRD, IAL>::pud_conjecture_initializer(
    IGNID& get_next_node_id,
    ISBG& store_body_goals,
    ISVC& store_var_count,
    IMR& mark_root,
    ISL& set_leaf,
    IMRD& make_root_descent,
    IAL& activate_leaf)
    : get_next_node_id_(get_next_node_id)
    , store_body_goals_(store_body_goals)
    , store_var_count_(store_var_count)
    , mark_root_(mark_root)
    , set_leaf_(set_leaf)
    , make_root_descent_(make_root_descent)
    , activate_leaf_(activate_leaf)
{}

template<typename IGNID, typename ISBG, typename ISVC, typename IMR, typename ISL, typename IMRD, typename IAL>
pud_node_id pud_conjecture_initializer<IGNID, ISBG, ISVC, IMR, ISL, IMRD, IAL>::initialize_conjecture(
    const std::vector<const expr*>& goals, uint32_t var_count) {
    const pud_node_id id = get_next_node_id_.next();
    store_body_goals_.store(id, goals);
    store_var_count_.store(id, var_count);
    mark_root_.mark_root(id);
    set_leaf_.set_leaf(id);
    activate_leaf_.activate(id, make_root_descent_.make(id));
    return id;
}

#endif
