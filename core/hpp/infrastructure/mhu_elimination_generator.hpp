#ifndef MHU_ELIMINATION_GENERATOR_HPP
#define MHU_ELIMINATION_GENERATOR_HPP

#include <cstdint>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include "debug_assert.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/unifier.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/unify_head.hpp"

template<typename IBindMap, typename IBindMapFactory, typename IUnifier,
         typename IUnifierFactory, typename IMakeResolutionLineage,
         typename IMakeVar, typename IGetGoalCandidateRuleIds>
struct mhu_elimination_generator {
    mhu_elimination_generator(IBindMap&, IMakeResolutionLineage&, IMakeVar&,
                               IBindMapFactory&, IUnifierFactory&,
                               const IGetGoalCandidateRuleIds&);
    bool try_add_head(const resolution_lineage*, framed_expr goal, framed_expr head);
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
    void clear_mhu_heads();
private:
    coroutine<const resolution_lineage*, void> accept_bindings(IBindMap&, const std::unordered_set<uint32_t>&);
    coroutine<const resolution_lineage*, void> rebase_all(uint32_t);
    bool sync_and_link(const resolution_lineage*, IUnifier&, std::queue<uint32_t>&);
    coroutine<uint32_t, bool> synchronize(IUnifier&, std::queue<uint32_t>&);
    void link(const std::unordered_set<uint32_t>&, const std::unordered_set<const resolution_lineage*>&);
    std::unordered_set<const resolution_lineage*> unlink(uint32_t);
    std::unordered_set<uint32_t> unlink(const resolution_lineage*);
    void remove_head(const resolution_lineage*);

    IBindMap& common_;
    IMakeResolutionLineage& make_resolution_lineage_;
    IMakeVar& make_var_;
    IBindMapFactory& bind_map_factory_;
    IUnifierFactory& unifier_factory_;
    const IGetGoalCandidateRuleIds& get_goal_candidate_rule_ids_;

    std::unordered_map<const resolution_lineage*, unify_head<IBindMap, IUnifier>> heads_;
    std::unordered_map<uint32_t, std::unordered_set<const resolution_lineage*>> rep_to_rls_;
    std::unordered_map<const resolution_lineage*, std::unordered_set<uint32_t>> rl_to_reps_;
};

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI>::mhu_elimination_generator(
    IBM& common, IMRL& mrl, IMV& mv, IBMF& bmf, IUF& uf, const IGCRI& gcri)
    : common_(common), make_resolution_lineage_(mrl), make_var_(mv),
      bind_map_factory_(bmf), unifier_factory_(uf),
      get_goal_candidate_rule_ids_(gcri) {}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
bool mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI>::try_add_head(
    const resolution_lineage* lineage, framed_expr lhs, framed_expr rhs) {
    auto bm = std::make_unique<IBM>(bind_map_factory_.make());
    IU u = unifier_factory_.make(bm.get());

    std::queue<uint32_t> touched_vars;
    auto task = u.unify(lhs, rhs);
    while (!task.done()) {
        task.resume();
        if (task.has_yield())
            touched_vars.push(task.consume_yield());
    }

    if (!task.result())
        return false;

    if (!sync_and_link(lineage, u, touched_vars))
        return false;

    unify_head<IBM, IU> head{std::move(bm), std::move(u)};
    const auto [_, inserted] = heads_.insert({lineage, std::move(head)});
    DEBUG_ASSERT(inserted);
    return true;
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
coroutine<const resolution_lineage*, void>
mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI>::constrain(
    const resolution_lineage* lineage) {
    auto gl = lineage->parent;
    auto& candidates = get_goal_candidate_rule_ids_.get(gl);
    auto it_sm = candidates.iterate();
    while (!it_sm.done()) {
        it_sm.resume();
        if (!it_sm.has_yield())
            continue;
        const rule_id candidate = it_sm.consume_yield();
        if (candidate == lineage->idx)
            continue;
        remove_head(make_resolution_lineage_.make_resolution_lineage(gl, candidate));
    }

    auto& head = heads_.at(lineage);
    auto c_reps = unlink(lineage);
    auto sm0 = accept_bindings(*head.local_bind_map, c_reps);
    while (!sm0.done()) {
        sm0.resume();
        if (sm0.has_yield())
            co_yield sm0.consume_yield();
    }

    heads_.erase(lineage);
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
void mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI>::clear_mhu_heads() {
    heads_.clear();
    rep_to_rls_.clear();
    rl_to_reps_.clear();
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
coroutine<const resolution_lineage*, void>
mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI>::accept_bindings(
    IBM& local_bind_map, const std::unordered_set<uint32_t>& c_reps) {
    for (auto c_rep : c_reps) {
        auto rep_expr = make_var_.make_var(c_rep);
        framed_expr whnf = local_bind_map.whnf({rep_expr, 0});

        if (rep_expr == whnf.skeleton && whnf.frame_offset == 0)
            continue;

        common_.bind(c_rep, whnf);

        auto sm0 = rebase_all(c_rep);
        while (!sm0.done()) {
            sm0.resume();
            if (sm0.has_yield())
                co_yield sm0.consume_yield();
        }
    }
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
coroutine<const resolution_lineage*, void>
mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI>::rebase_all(uint32_t rep) {
    auto remaining_rls = unlink(rep);
    for (auto rl : remaining_rls) {
        std::queue<uint32_t> touched_vars;
        touched_vars.push(rep);
        if (!sync_and_link(rl, heads_.at(rl).unifier, touched_vars)) {
            remove_head(rl);
            co_yield rl;
        }
    }
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
bool mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI>::sync_and_link(
    const resolution_lineage* lineage, IU& unifier, std::queue<uint32_t>& touched_vars) {
    std::unordered_set<uint32_t> c_reps;
    auto sync_task = synchronize(unifier, touched_vars);
    while (!sync_task.done()) {
        sync_task.resume();
        if (sync_task.has_yield())
            c_reps.insert(sync_task.consume_yield());
    }
    if (!sync_task.result())
        return false;
    link(c_reps, {lineage});
    return true;
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
coroutine<uint32_t, bool>
mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI>::synchronize(
    IU& unifier, std::queue<uint32_t>& touched_vars) {
    while (!touched_vars.empty()) {
        auto var = touched_vars.front();
        touched_vars.pop();

        auto var_expr = make_var_.make_var(var);
        framed_expr whnf = common_.whnf({var_expr, 0});

        if (var_expr == whnf.skeleton && whnf.frame_offset == 0) {
            co_yield var;
            continue;
        }

        auto task = unifier.unify({var_expr, 0}, whnf);
        while (!task.done()) {
            task.resume();
            if (task.has_yield())
                touched_vars.push(task.consume_yield());
        }

        if (!task.result())
            co_return false;
    }
    co_return true;
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
void mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI>::link(
    const std::unordered_set<uint32_t>& reps,
    const std::unordered_set<const resolution_lineage*>& rls) {
    for (auto rep : reps)
        rep_to_rls_[rep].insert(rls.begin(), rls.end());
    for (auto rl : rls)
        rl_to_reps_[rl].insert(reps.begin(), reps.end());
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
std::unordered_set<const resolution_lineage*>
mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI>::unlink(uint32_t rep) {
    auto node = rep_to_rls_.extract(rep);
    if (node.empty())
        return {};

    auto& rls = node.mapped();
    for (auto rl : rls) {
        auto& reps = rl_to_reps_.at(rl);
        reps.erase(rep);
        if (reps.empty())
            rl_to_reps_.erase(rl);
    }
    return std::move(rls);
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
std::unordered_set<uint32_t>
mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI>::unlink(
    const resolution_lineage* rl) {
    auto node = rl_to_reps_.extract(rl);
    if (node.empty())
        return {};

    auto& reps = node.mapped();
    for (auto rep : reps) {
        auto& heads = rep_to_rls_.at(rep);
        heads.erase(rl);
        if (heads.empty())
            rep_to_rls_.erase(rep);
    }
    return std::move(reps);
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
void mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI>::remove_head(
    const resolution_lineage* rl) {
    heads_.erase(rl);
    unlink(rl);
}

#endif
