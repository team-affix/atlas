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
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

template<typename IAllocateChildInterval,
         typename IFindParent,
         typename IGetAddedUnifications,
         typename IRecordBinding,
         typename IQueryBinding,
         typename IGlobalize,
         typename IMakeVar,
         typename IMakeFunctor>
struct pud_unify_head {
    pud_unify_head(IAllocateChildInterval& allocate_child_interval,
                   IFindParent& find_parent,
                   IGetAddedUnifications& get_added_unifications,
                   IRecordBinding& record_binding,
                   IQueryBinding& query_binding,
                   IGlobalize& globalize,
                   IMakeVar& make_var,
                   IMakeFunctor& make_functor);
    bool unify_head(pud_query& query, const pud_rule_id* node);
    bool unify_callee(pud_query& query,
                      const pud_rule_id* callee,
                      std::vector<uint32_t>& touched_reps,
                      om_interval& env);
    const expr* normalize(om_interval interval,
                          framed_expr fe,
                          uint32_t cutoff,
                          std::unordered_map<uint32_t, uint32_t>& translation);
    void drop_query(pud_query* query);
private:
    using bind_map_t = hierarchical_bind_map<IGlobalize, IRecordBinding, IQueryBinding>;
    using unifier_t = unifier<IGlobalize, bind_map_t>;
    using normalizer_t = normalizer<IGlobalize, IMakeFunctor, IMakeVar, bind_map_t>;

    struct env_row {
        om_interval interval;
        std::vector<uint32_t> touched_reps;
        enum class unify_state { pending, ok, failed };
        unify_state state;
    };
    using node_env_map_t = std::unordered_map<const pud_rule_id*, env_row>;
    using query_env_map_t = std::unordered_map<const pud_query*, node_env_map_t>;

    om_interval ensure(pud_query& query, const pud_rule_id* node);
    env_row& row_for(pud_query& query, const pud_rule_id* node);
    bool try_unify_row(pud_query& query, env_row& row);
    bool drain_unify(unifier_t& task_owner,
                     framed_expr lhs,
                     framed_expr rhs,
                     std::vector<uint32_t>* touched_reps);

    IAllocateChildInterval& allocate_child_interval_;
    IFindParent& find_parent_;
    IGetAddedUnifications& get_added_unifications_;
    IRecordBinding& record_binding_;
    IQueryBinding& query_binding_;
    IGlobalize& globalize_;
    IMakeVar& make_var_;
    IMakeFunctor& make_functor_;
    query_env_map_t envs_;
};

template<typename IACI, typename IFP, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::pud_unify_head(
        IACI& allocate_child_interval,
        IFP& find_parent,
        IGAU& get_added_unifications,
        IRB& record_binding,
        IQB& query_binding,
        IG& globalize,
        IMV& make_var,
        IMF& make_functor)
    : allocate_child_interval_(allocate_child_interval)
    , find_parent_(find_parent)
    , get_added_unifications_(get_added_unifications)
    , record_binding_(record_binding)
    , query_binding_(query_binding)
    , globalize_(globalize)
    , make_var_(make_var)
    , make_functor_(make_functor)
    , envs_() {}

template<typename IACI, typename IFP, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
om_interval pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::ensure(
        pud_query& query, const pud_rule_id* node) {
    node_env_map_t& by_node = envs_[&query];
    auto it = by_node.find(node);
    if (it != by_node.end())
        return it->second.interval;
    const pud_rule_id* parent = find_parent_.find_parent(node);
    const om_interval parent_env = parent != nullptr
        ? ensure(query, parent)
        : query.interval;
    const om_interval env = allocate_child_interval_.allocate_child_of(parent_env);
    for (const pud_added_unification& added : get_added_unifications_.get(node))
        record_binding_.record(env, added.var_idx,
                               framed_expr{added.value, query.frame_offset});
    by_node.emplace(node, env_row{
        env,
        {},
        env_row::unify_state::pending});
    return env;
}

template<typename IACI, typename IFP, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
typename pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::env_row&
pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::row_for(
        pud_query& query, const pud_rule_id* node) {
    ensure(query, node);
    return envs_[&query].at(node);
}

template<typename IACI, typename IFP, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
bool pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::drain_unify(
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

template<typename IACI, typename IFP, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
bool pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::try_unify_row(
        pud_query& query, env_row& row) {
    if (row.state == env_row::unify_state::ok)
        return true;
    if (row.state == env_row::unify_state::failed)
        return false;
    bind_map_t bm(globalize_, record_binding_, query_binding_, row.interval);
    unifier_t u(globalize_, &bm);
    const framed_expr body{query.body_goal, query.frame_offset};
    const framed_expr head{make_var_.make_var(0), 0};
    const bool ok = drain_unify(u, body, head, &row.touched_reps);
    if (ok)
        row.state = env_row::unify_state::ok;
    else
        row.state = env_row::unify_state::failed;
    return ok;
}

template<typename IACI, typename IFP, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
bool pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::unify_head(
        pud_query& query, const pud_rule_id* node) {
    return try_unify_row(query, row_for(query, node));
}

template<typename IACI, typename IFP, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
bool pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::unify_callee(
        pud_query& query,
        const pud_rule_id* callee,
        std::vector<uint32_t>& touched_reps,
        om_interval& env) {
    env_row& row = row_for(query, callee);
    const bool ok = try_unify_row(query, row);
    env = row.interval;
    if (ok)
        touched_reps = row.touched_reps;
    return ok;
}

template<typename IACI, typename IFP, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
const expr* pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::normalize(
        om_interval interval,
        framed_expr fe,
        uint32_t cutoff,
        std::unordered_map<uint32_t, uint32_t>& translation) {
    bind_map_t bm(globalize_, record_binding_, query_binding_, interval);
    normalizer_t n(globalize_, make_functor_, make_var_, bm);
    return n.normalize(fe, cutoff, translation);
}

template<typename IACI, typename IFP, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
void pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::drop_query(pud_query* query) {
    envs_.erase(query);
}

#endif
