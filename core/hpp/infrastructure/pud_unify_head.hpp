#ifndef PUD_UNIFY_HEAD_HPP
#define PUD_UNIFY_HEAD_HPP

#include <cstdint>
#include <unordered_map>
#include <vector>
#include "infrastructure/hierarchical_bind_map.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/unifier.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

template<typename IAllocateChildInterval,
         typename ITryParent,
         typename IGetNode,
         typename IRecordBinding,
         typename IQueryBinding,
         typename IGlobalize,
         typename IMakeVar,
         typename IMakeFunctor>
struct pud_unify_head {
    pud_unify_head(IAllocateChildInterval& allocate_child_interval,
                   ITryParent& try_parent,
                   IGetNode& get_node,
                   IRecordBinding& record_binding,
                   IQueryBinding& query_binding,
                   IGlobalize& globalize,
                   IMakeVar& make_var,
                   IMakeFunctor& make_functor);
    bool unify_head(pud_query& query, const pud_rule_id* node);
    bool unify(pud_query& query, framed_expr leftover_body);
    framed_expr whnf(pud_query& query, framed_expr fe);
    bool unify_callee(pud_query& query,
                      const pud_rule_id* callee,
                      std::vector<uint32_t>& touched_reps,
                      om_interval& env);
    const expr* normalize(pud_query& query,
                          om_interval interval,
                          framed_expr fe,
                          uint32_t cutoff,
                          std::unordered_map<uint32_t, uint32_t>& translation);
    void reinit(pud_query& query);
private:
    using bind_map_t = hierarchical_bind_map<IGlobalize, IRecordBinding, IQueryBinding>;
    using unifier_t = unifier<IGlobalize, bind_map_t>;
    using normalizer_t = normalizer<IGlobalize, IMakeFunctor, IMakeVar, bind_map_t>;

    void replay_raw(om_interval interval, const pud_rule_id* node, uint32_t frame_offset);
    void replay_reinit(pud_query& query, const pud_rule_id* node);
    bool drain_unify(unifier_t& task_owner,
                     framed_expr lhs,
                     framed_expr rhs,
                     std::vector<uint32_t>* touched_reps);

    IAllocateChildInterval& allocate_child_interval_;
    ITryParent& try_parent_;
    IGetNode& get_node_;
    IRecordBinding& record_binding_;
    IQueryBinding& query_binding_;
    IGlobalize& globalize_;
    IMakeVar& make_var_;
    IMakeFunctor& make_functor_;
};

template<typename IACI, typename ITP, typename IGN, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
pud_unify_head<IACI, ITP, IGN, IRB, IQB, IG, IMV, IMF>::pud_unify_head(
        IACI& allocate_child_interval,
        ITP& try_parent,
        IGN& get_node,
        IRB& record_binding,
        IQB& query_binding,
        IG& globalize,
        IMV& make_var,
        IMF& make_functor)
    : allocate_child_interval_(allocate_child_interval)
    , try_parent_(try_parent)
    , get_node_(get_node)
    , record_binding_(record_binding)
    , query_binding_(query_binding)
    , globalize_(globalize)
    , make_var_(make_var)
    , make_functor_(make_functor) {}

template<typename IACI, typename ITP, typename IGN, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
void pud_unify_head<IACI, ITP, IGN, IRB, IQB, IG, IMV, IMF>::replay_raw(
        om_interval interval, const pud_rule_id* node, uint32_t frame_offset) {
    std::vector<const pud_rule_id*> path;
    const pud_rule_id* walk = node;
    while (walk != nullptr) {
        path.push_back(walk);
        walk = try_parent_.try_parent(walk);
    }
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        const pud_db_node& db_node = get_node_.get_node(*it);
        for (const pud_added_unification& added : db_node.added_unifications)
            record_binding_.record(interval, added.var_idx,
                                   framed_expr{added.value, frame_offset});
    }
}

template<typename IACI, typename ITP, typename IGN, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
void pud_unify_head<IACI, ITP, IGN, IRB, IQB, IG, IMV, IMF>::replay_reinit(
        pud_query& query, const pud_rule_id* node) {
    std::vector<const pud_rule_id*> path;
    const pud_rule_id* walk = node;
    while (walk != nullptr) {
        path.push_back(walk);
        walk = try_parent_.try_parent(walk);
    }
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        const pud_db_node& db_node = get_node_.get_node(*it);
        for (const pud_added_unification& added : db_node.added_unifications) {
            const framed_expr value =
                whnf(query, framed_expr{added.value, query.frame_offset});
            record_binding_.record(query.interval, added.var_idx, value);
        }
        if (!unify(query, framed_expr{query.body_goal, query.frame_offset}))
            return;
    }
}

template<typename IACI, typename ITP, typename IGN, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
bool pud_unify_head<IACI, ITP, IGN, IRB, IQB, IG, IMV, IMF>::drain_unify(
        unifier_t& task_owner,
        framed_expr lhs,
        framed_expr rhs,
        std::vector<uint32_t>* touched_reps) {
    auto task = task_owner.unify(lhs, rhs);
    while (!task.done()) {
        task.resume();
        if (!task.has_yield())
            continue;
        const uint32_t rep = task.consume_yield();
        if (touched_reps != nullptr)
            touched_reps->push_back(rep);
    }
    return task.result();
}

template<typename IACI, typename ITP, typename IGN, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
bool pud_unify_head<IACI, ITP, IGN, IRB, IQB, IG, IMV, IMF>::unify_head(
        pud_query& query, const pud_rule_id* node) {
    const om_interval nested = allocate_child_interval_.allocate_child_of(query.interval);
    replay_raw(nested, node, query.frame_offset);
    bind_map_t bm(globalize_, record_binding_, query_binding_, nested);
    unifier_t u(globalize_, &bm);
    const framed_expr body{query.body_goal, query.frame_offset};
    const framed_expr head{make_var_.make_var(0), 0};
    return drain_unify(u, body, head, nullptr);
}

template<typename IACI, typename ITP, typename IGN, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
bool pud_unify_head<IACI, ITP, IGN, IRB, IQB, IG, IMV, IMF>::unify(
        pud_query& query, framed_expr leftover_body) {
    bind_map_t bm(globalize_, record_binding_, query_binding_, query.interval);
    unifier_t u(globalize_, &bm);
    const framed_expr head{make_var_.make_var(0), 0};
    return drain_unify(u, leftover_body, head, nullptr);
}

template<typename IACI, typename ITP, typename IGN, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
framed_expr pud_unify_head<IACI, ITP, IGN, IRB, IQB, IG, IMV, IMF>::whnf(
        pud_query& query, framed_expr fe) {
    bind_map_t bm(globalize_, record_binding_, query_binding_, query.interval);
    return bm.whnf(fe);
}

template<typename IACI, typename ITP, typename IGN, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
bool pud_unify_head<IACI, ITP, IGN, IRB, IQB, IG, IMV, IMF>::unify_callee(
        pud_query& query,
        const pud_rule_id* callee,
        std::vector<uint32_t>& touched_reps,
        om_interval& env) {
    env = allocate_child_interval_.allocate_child_of(query.interval);
    replay_raw(env, callee, query.frame_offset);
    bind_map_t bm(globalize_, record_binding_, query_binding_, env);
    unifier_t u(globalize_, &bm);
    const framed_expr body{query.body_goal, query.frame_offset};
    const framed_expr head{make_var_.make_var(0), 0};
    return drain_unify(u, body, head, &touched_reps);
}

template<typename IACI, typename ITP, typename IGN, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
const expr* pud_unify_head<IACI, ITP, IGN, IRB, IQB, IG, IMV, IMF>::normalize(
        pud_query& query,
        om_interval interval,
        framed_expr fe,
        uint32_t cutoff,
        std::unordered_map<uint32_t, uint32_t>& translation) {
    (void)query;
    bind_map_t bm(globalize_, record_binding_, query_binding_, interval);
    normalizer_t n(globalize_, make_functor_, make_var_, bm);
    return n.normalize(fe, cutoff, translation);
}

template<typename IACI, typename ITP, typename IGN, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
void pud_unify_head<IACI, ITP, IGN, IRB, IQB, IG, IMV, IMF>::reinit(pud_query& query) {
    for (pud_candidate_search_context& axiom_ctx : query.axiom_contexts) {
        replay_reinit(query, axiom_ctx.cursor);
        for (const pud_witness_search_context& edge : axiom_ctx.live_edges)
            replay_reinit(query, edge.current);
    }
}

#endif
