#ifndef PUD_NODE_INDEXER_HPP
#define PUD_NODE_INDEXER_HPP

#include "value_objects/expr.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_node_delta.hpp"
#include "value_objects/rule.hpp"

// Records what follows from a stored node: its open goals and its lvc. An axiom's goals are its body.
// An inference is its parent's rule resolved on one of its goals, so it keeps every other goal of the
// parent, under the same ids, and adds its own. A node is indexed after its parent.
template<
    typename IGetAxiom,
    typename IGetInference,
    typename IGetGoals,
    typename IStoreGoals,
    typename IGetLvc,
    typename IStoreLvc>
struct pud_node_indexer {
    pud_node_indexer(
        IGetAxiom& get_axiom,
        IGetInference& get_inference,
        IGetGoals& get_goals,
        IStoreGoals& store_goals,
        IGetLvc& get_lvc,
        IStoreLvc& store_lvc);
    void index_axiom(pud_global_node_id axiom);
    void index_inference(pud_global_node_id inference);
private:
    IGetAxiom& get_axiom_;
    IGetInference& get_inference_;
    IGetGoals& get_goals_;
    IStoreGoals& store_goals_;
    IGetLvc& get_lvc_;
    IStoreLvc& store_lvc_;
};

#define PUD_NODE_INDEXER_TEMPLATE                                                          \
    template<typename IGetAxiom, typename IGetInference, typename IGetGoals, typename IStoreGoals, \
             typename IGetLvc, typename IStoreLvc>
#define PUD_NODE_INDEXER pud_node_indexer<IGetAxiom, IGetInference, IGetGoals, IStoreGoals, IGetLvc, IStoreLvc>

PUD_NODE_INDEXER_TEMPLATE
PUD_NODE_INDEXER::pud_node_indexer(
    IGetAxiom& get_axiom,
    IGetInference& get_inference,
    IGetGoals& get_goals,
    IStoreGoals& store_goals,
    IGetLvc& get_lvc,
    IStoreLvc& store_lvc)
    : get_axiom_(get_axiom)
    , get_inference_(get_inference)
    , get_goals_(get_goals)
    , store_goals_(store_goals)
    , get_lvc_(get_lvc)
    , store_lvc_(store_lvc) {}

PUD_NODE_INDEXER_TEMPLATE
void PUD_NODE_INDEXER::index_axiom(pud_global_node_id axiom) {
    const rule& indexed = get_axiom_.get_axiom(axiom);
    pud_goal_list goals{.goals = {}, .next_id = 0};
    for (const expr* goal : indexed.body)
        goals = goals.adding(goal);
    store_goals_.store(axiom, goals);
    store_lvc_.store(axiom, indexed.var_count);
}

PUD_NODE_INDEXER_TEMPLATE
void PUD_NODE_INDEXER::index_inference(pud_global_node_id inference) {
    const pud_node_delta& delta = get_inference_.get_inference(inference);
    const pud_global_node_id parent{.module = inference.module, .node = delta.parent};
    pud_goal_list goals = get_goals_.get(parent).closing(delta.call_site);
    for (const expr* goal : delta.body_goals)
        goals = goals.adding(goal);
    store_goals_.store(inference, goals);
    store_lvc_.store(inference, get_lvc_.get(parent) + delta.var_count);
}

#undef PUD_NODE_INDEXER
#undef PUD_NODE_INDEXER_TEMPLATE

#endif
