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
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

template<typename IAllocateChildInterval,
         typename IGetParent,
         typename IGetAddedUnifications,
         typename IRecordBinding,
         typename IQueryBinding,
         typename IGlobalize,
         typename IMakeVar,
         typename IMakeFunctor>
struct pud_unify_head {
    pud_unify_head(IAllocateChildInterval& allocate_child_interval,
                   IGetParent& get_parent,
                   IGetAddedUnifications& get_added_unifications,
                   IRecordBinding& record_binding,
                   IQueryBinding& query_binding,
                   IGlobalize& globalize,
                   IMakeVar& make_var,
                   IMakeFunctor& make_functor);
    template<typename IEnv>
    bool unify_head(IEnv& env, const pud_rule_id* node);
    template<typename IEnv>
    bool unify_callee(IEnv& env,
                      const pud_rule_id* callee,
                      std::vector<uint32_t>& touched_reps,
                      om_interval& env_out);
    const expr* normalize(om_interval interval,
                          framed_expr fe,
                          uint32_t cutoff,
                          std::unordered_map<uint32_t, uint32_t>& translation);
    void drop_env(om_interval interval);
private:
    using bind_map_t = hierarchical_bind_map<IGlobalize, IRecordBinding, IQueryBinding>;
    using unifier_t = unifier<IGlobalize, bind_map_t>;
    using normalizer_t = normalizer<IGlobalize, IMakeFunctor, IMakeVar, bind_map_t>;
    using env_key_t = const uint64_t*;

    struct env_row {
        om_interval interval;
        std::vector<uint32_t> touched_reps;
        enum class unify_state { pending, ok, failed };
        unify_state state;
    };
    using node_env_map_t = std::unordered_map<const pud_rule_id*, env_row>;
    using query_env_map_t = std::unordered_map<env_key_t, node_env_map_t>;

    template<typename IEnv>
    om_interval ensure(IEnv& env, const pud_rule_id* node);
    template<typename IEnv>
    env_row& row_for(IEnv& env, const pud_rule_id* node);
    template<typename IEnv>
    bool try_unify_row(IEnv& env, env_row& row);
    bool drain_unify(unifier_t& task_owner,
                     framed_expr lhs,
                     framed_expr rhs,
                     std::vector<uint32_t>* touched_reps);

    IAllocateChildInterval& allocate_child_interval_;
    IGetParent& get_parent_;
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
        IFP& get_parent,
        IGAU& get_added_unifications,
        IRB& record_binding,
        IQB& query_binding,
        IG& globalize,
        IMV& make_var,
        IMF& make_functor)
    : allocate_child_interval_(allocate_child_interval)
    , get_parent_(get_parent)
    , get_added_unifications_(get_added_unifications)
    , record_binding_(record_binding)
    , query_binding_(query_binding)
    , globalize_(globalize)
    , make_var_(make_var)
    , make_functor_(make_functor)
    , envs_() {}

template<typename IACI, typename IFP, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
template<typename IEnv>
om_interval pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::ensure(
        IEnv& env, const pud_rule_id* node) {
    node_env_map_t& by_node = envs_[env.interval.open.rank_ptr()];
    auto it = by_node.find(node);
    if (it != by_node.end())
        return it->second.interval;
    const pud_rule_id* parent = get_parent_.get(node);
    const om_interval parent_env = parent != nullptr
        ? ensure(env, parent)
        : env.interval;
    const om_interval child_env = allocate_child_interval_.allocate_child_of(parent_env);
    for (const pud_added_unification& added : get_added_unifications_.get(node))
        record_binding_.record(child_env, added.var_idx,
                               framed_expr{added.value, env.frame_offset});
    by_node.emplace(node, env_row{
        child_env,
        {},
        env_row::unify_state::pending});
    return child_env;
}

template<typename IACI, typename IFP, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
template<typename IEnv>
typename pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::env_row&
pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::row_for(
        IEnv& env, const pud_rule_id* node) {
    ensure(env, node);
    return envs_[env.interval.open.rank_ptr()].at(node);
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
template<typename IEnv>
bool pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::try_unify_row(
        IEnv& env, env_row& row) {
    if (row.state == env_row::unify_state::ok)
        return true;
    if (row.state == env_row::unify_state::failed)
        return false;
    bind_map_t bm(globalize_, record_binding_, query_binding_, row.interval);
    unifier_t u(globalize_, &bm);
    const framed_expr body{env.body_goal, env.frame_offset};
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
template<typename IEnv>
bool pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::unify_head(
        IEnv& env, const pud_rule_id* node) {
    return try_unify_row(env, row_for(env, node));
}

template<typename IACI, typename IFP, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename IMF>
template<typename IEnv>
bool pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::unify_callee(
        IEnv& env,
        const pud_rule_id* callee,
        std::vector<uint32_t>& touched_reps,
        om_interval& env_out) {
    env_row& row = row_for(env, callee);
    const bool ok = try_unify_row(env, row);
    env_out = row.interval;
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
void pud_unify_head<IACI, IFP, IGAU, IRB, IQB, IG, IMV, IMF>::drop_env(om_interval interval) {
    envs_.erase(interval.open.rank_ptr());
}

#endif
