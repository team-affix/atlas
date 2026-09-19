#ifndef PUD_QUERY_REINITIALIZER_HPP
#define PUD_QUERY_REINITIALIZER_HPP

#include <vector>
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

template<typename ITryParent, typename IGetNode, typename IRecordBinding,
         typename IWhnf, typename IUnify>
struct pud_query_reinitializer {
    pud_query_reinitializer(ITryParent& try_parent,
                            IGetNode& get_node,
                            IRecordBinding& record_binding,
                            IWhnf& whnf,
                            IUnify& unify);
    void reinit(pud_query& query, uint32_t frame_offset);
private:
    void replay_path(pud_query& query, const pud_rule_id* node, uint32_t frame_offset);

    ITryParent& try_parent_;
    IGetNode& get_node_;
    IRecordBinding& record_binding_;
    IWhnf& whnf_;
    IUnify& unify_;
};

template<typename ITP, typename IGN, typename IRB, typename IW, typename IU>
pud_query_reinitializer<ITP, IGN, IRB, IW, IU>::pud_query_reinitializer(
        ITP& try_parent,
        IGN& get_node,
        IRB& record_binding,
        IW& whnf,
        IU& unify)
    : try_parent_(try_parent)
    , get_node_(get_node)
    , record_binding_(record_binding)
    , whnf_(whnf)
    , unify_(unify) {}

template<typename ITP, typename IGN, typename IRB, typename IW, typename IU>
void pud_query_reinitializer<ITP, IGN, IRB, IW, IU>::replay_path(
        pud_query& query,
        const pud_rule_id* node,
        uint32_t frame_offset) {
    std::vector<const pud_rule_id*> path;
    const pud_rule_id* walk = node;
    while (walk != nullptr) {
        path.push_back(walk);
        walk = try_parent_.try_parent(walk);
    }
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        const pud_db_node& db_node = get_node_.get_node(*it);
        for (const pud_added_unification& added : db_node.added_unifications) {
            const framed_expr value = whnf_.whnf(framed_expr{added.value, frame_offset});
            record_binding_.record(query.interval, added.var_idx, value);
        }
        if (!unify_.unify(framed_expr{query.body_goal, frame_offset}))
            return;
    }
}

template<typename ITP, typename IGN, typename IRB, typename IW, typename IU>
void pud_query_reinitializer<ITP, IGN, IRB, IW, IU>::reinit(
        pud_query& query, uint32_t frame_offset) {
    for (pud_candidate_search_context& axiom_ctx : query.axiom_contexts) {
        replay_path(query, axiom_ctx.cursor, frame_offset);
        for (const pud_witness_search_context& edge : axiom_ctx.live_edges)
            replay_path(query, edge.current, frame_offset);
    }
}

#endif
