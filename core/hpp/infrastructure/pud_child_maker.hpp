#ifndef PUD_CHILD_MAKER_HPP
#define PUD_CHILD_MAKER_HPP

#include "value_objects/pud_child_delta.hpp"
#include "value_objects/pud_node_id.hpp"

template<
    typename IGetNextNodeID,
    typename IStoreSubstitutions,
    typename IStoreBodyGoals,
    typename IStoreVarCount,
    typename IStoreParent,
    typename ISetLeaf>
struct pud_child_maker {
    pud_child_maker(
        IGetNextNodeID& get_next_node_id,
        IStoreSubstitutions& store_substitutions,
        IStoreBodyGoals& store_body_goals,
        IStoreVarCount& store_var_count,
        IStoreParent& store_parent,
        ISetLeaf& set_leaf);
    pud_node_id make(pud_node_id parent, const pud_child_delta& delta);
private:
    IGetNextNodeID& get_next_node_id_;
    IStoreSubstitutions& store_substitutions_;
    IStoreBodyGoals& store_body_goals_;
    IStoreVarCount& store_var_count_;
    IStoreParent& store_parent_;
    ISetLeaf& set_leaf_;
};

#define PUD_CHILD_MAKER_TEMPLATE                                                           \
    template<typename IGetNextNodeID, typename IStoreSubstitutions, typename IStoreBodyGoals, \
             typename IStoreVarCount, typename IStoreParent, typename ISetLeaf>
#define PUD_CHILD_MAKER                                                                    \
    pud_child_maker<IGetNextNodeID, IStoreSubstitutions, IStoreBodyGoals, IStoreVarCount,  \
        IStoreParent, ISetLeaf>

PUD_CHILD_MAKER_TEMPLATE
PUD_CHILD_MAKER::pud_child_maker(
    IGetNextNodeID& get_next_node_id,
    IStoreSubstitutions& store_substitutions,
    IStoreBodyGoals& store_body_goals,
    IStoreVarCount& store_var_count,
    IStoreParent& store_parent,
    ISetLeaf& set_leaf)
    : get_next_node_id_(get_next_node_id)
    , store_substitutions_(store_substitutions)
    , store_body_goals_(store_body_goals)
    , store_var_count_(store_var_count)
    , store_parent_(store_parent)
    , set_leaf_(set_leaf) {}

PUD_CHILD_MAKER_TEMPLATE
pud_node_id PUD_CHILD_MAKER::make(pud_node_id parent, const pud_child_delta& delta) {
    const pud_node_id child = get_next_node_id_.next();
    store_substitutions_.store(child, delta.substitutions);
    store_body_goals_.store(child, delta.body_goals);
    store_var_count_.store(child, delta.var_count);
    store_parent_.store(child, parent);
    set_leaf_.set_leaf(child);
    return child;
}

#undef PUD_CHILD_MAKER
#undef PUD_CHILD_MAKER_TEMPLATE

#endif
