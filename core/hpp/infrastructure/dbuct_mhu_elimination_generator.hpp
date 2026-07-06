#ifndef DBUCT_MHU_ELIMINATION_GENERATOR_HPP
#define DBUCT_MHU_ELIMINATION_GENERATOR_HPP

#include <cstdint>
#include <deque>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include "infrastructure/backtrackable_deque_emplace_back.hpp"
#include "infrastructure/backtrackable_map_at_erase.hpp"
#include "infrastructure/backtrackable_map_at_insert.hpp"
#include "infrastructure/backtrackable_map_erase.hpp"
#include "infrastructure/backtrackable_map_insert.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/tracked.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/unify_head.hpp"

// Delayed-backtracking variant of mhu_elimination_generator, fully journalled on
// the trail (supplied as the abstract ILogTrailAction, not a concrete trail); no
// snapshot/restore, no deep clone.
//
// The one hard structure: each head owns a unique_ptr<local bind_map> and a
// unifier that holds a raw pointer INTO that bind map. The heap-allocated bind
// map keeps a stable address for the head's whole lifetime, so the challenge is
// purely lifetime management of the head object.
//
// Design:
//   * Heads live in a std::deque arena (end-ops never relocate existing
//     elements), and each creation logs a backtrackable_deque_emplace_back whose
//     undo pop_back destroys the slot. heads_ maps lineage -> unify_head* and is
//     journalled with map insert/erase over the (copyable) pointer.
//   * Logical head removal only journal-erases the heads_ pointer entry and the
//     rep/rl link-table entries; the arena slot's physical destruction is
//     DEFERRED to the creation frame's pop. LIFO guarantees every bind-undo and
//     link-undo that references a head runs before that head's emplace_back undo,
//     so no undo ever dangles.
//   * A head's creation-time binds run with the local bind map's journaling OFF
//     (they are subsumed by the arena undo that destroys the whole map);
//     enable_journaling() is flipped on once the head commits, so later rebase
//     binds are individually reversible. Common-substitution binds journal on the
//     common bind map (always journaling), which lives for the whole solve.
template<typename IBindMap, typename IBindMapFactory, typename IUnifier,
         typename IUnifierFactory, typename IMakeResolutionLineage,
         typename IMakeVar, typename IGetGoalCandidateRuleIds,
         typename ILogTrailAction>
struct dbuct_mhu_elimination_generator {
    dbuct_mhu_elimination_generator(IBindMap&, IMakeResolutionLineage&, IMakeVar&,
                                    IBindMapFactory&, IUnifierFactory&,
                                    const IGetGoalCandidateRuleIds&, ILogTrailAction&);

    bool try_add_head(const resolution_lineage*, framed_expr goal, framed_expr head);
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);

private:
    using head_t        = unify_head<IBindMap, IUnifier>;
    using arena_t       = std::deque<head_t>;
    using heads_map_t   = std::unordered_map<const resolution_lineage*, head_t*>;
    using rep_to_rls_t  = std::unordered_map<uint32_t, std::unordered_set<const resolution_lineage*>>;
    using rl_to_reps_t  = std::unordered_map<const resolution_lineage*, std::unordered_set<uint32_t>>;

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
    ILogTrailAction& trail_;

    arena_t arena_;
    tracked<heads_map_t, ILogTrailAction> heads_;
    tracked<rep_to_rls_t, ILogTrailAction> rep_to_rls_;
    tracked<rl_to_reps_t, ILogTrailAction> rl_to_reps_;
};

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename ILog>
dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, ILog>::dbuct_mhu_elimination_generator(
    IBM& common, IMRL& mrl, IMV& mv, IBMF& bmf, IUF& uf, const IGCRI& gcri, ILog& t)
    : common_(common), make_resolution_lineage_(mrl), make_var_(mv),
      bind_map_factory_(bmf), unifier_factory_(uf),
      get_goal_candidate_rule_ids_(gcri), trail_(t),
      heads_(t, heads_map_t{}), rep_to_rls_(t, rep_to_rls_t{}), rl_to_reps_(t, rl_to_reps_t{}) {}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename ILog>
bool dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, ILog>::try_add_head(
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

    // The head commits: journal its subsequent (rebase) binds. Creation binds
    // above are subsumed by the arena's own undo.
    bm->enable_journaling();

    head_t head{std::move(bm), std::move(u)};
    auto emplace = std::make_unique<backtrackable_deque_emplace_back<arena_t>>(std::move(head));
    emplace->capture(arena_);
    emplace->invoke();
    trail_.log(std::move(emplace));

    head_t* head_ptr = &arena_.back();
    heads_.mutate(std::make_unique<backtrackable_map_insert<heads_map_t>>(lineage, head_ptr));
    return true;
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename ILog>
coroutine<const resolution_lineage*, void>
dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, ILog>::constrain(
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

    head_t* head = heads_.get().at(lineage);
    auto c_reps = unlink(lineage);
    auto sm0 = accept_bindings(*head->local_bind_map, c_reps);
    while (!sm0.done()) {
        sm0.resume();
        if (sm0.has_yield())
            co_yield sm0.consume_yield();
    }

    if (heads_.get().contains(lineage))
        heads_.mutate(std::make_unique<backtrackable_map_erase<heads_map_t>>(lineage));
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename ILog>
coroutine<const resolution_lineage*, void>
dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, ILog>::accept_bindings(
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

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename ILog>
coroutine<const resolution_lineage*, void>
dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, ILog>::rebase_all(uint32_t rep) {
    auto remaining_rls = unlink(rep);
    for (auto rl : remaining_rls) {
        std::queue<uint32_t> touched_vars;
        touched_vars.push(rep);
        if (!sync_and_link(rl, heads_.get().at(rl)->unifier, touched_vars)) {
            remove_head(rl);
            co_yield rl;
        }
    }
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename ILog>
bool dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, ILog>::sync_and_link(
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

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename ILog>
coroutine<uint32_t, bool>
dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, ILog>::synchronize(
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

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename ILog>
void dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, ILog>::link(
    const std::unordered_set<uint32_t>& reps,
    const std::unordered_set<const resolution_lineage*>& rls) {
    for (auto rep : reps) {
        if (!rep_to_rls_.get().contains(rep))
            rep_to_rls_.mutate(std::make_unique<backtrackable_map_insert<rep_to_rls_t>>(
                rep, std::unordered_set<const resolution_lineage*>{}));
        for (auto rl : rls)
            if (!rep_to_rls_.get().at(rep).contains(rl))
                rep_to_rls_.mutate(std::make_unique<backtrackable_map_at_insert<rep_to_rls_t>>(rep, rl));
    }
    for (auto rl : rls) {
        if (!rl_to_reps_.get().contains(rl))
            rl_to_reps_.mutate(std::make_unique<backtrackable_map_insert<rl_to_reps_t>>(
                rl, std::unordered_set<uint32_t>{}));
        for (auto rep : reps)
            if (!rl_to_reps_.get().at(rl).contains(rep))
                rl_to_reps_.mutate(std::make_unique<backtrackable_map_at_insert<rl_to_reps_t>>(rl, rep));
    }
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename ILog>
std::unordered_set<const resolution_lineage*>
dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, ILog>::unlink(uint32_t rep) {
    if (!rep_to_rls_.get().contains(rep))
        return {};

    std::unordered_set<const resolution_lineage*> rls = rep_to_rls_.get().at(rep);
    for (auto rl : rls) {
        rl_to_reps_.mutate(std::make_unique<backtrackable_map_at_erase<rl_to_reps_t>>(rl, rep));
        if (rl_to_reps_.get().at(rl).empty())
            rl_to_reps_.mutate(std::make_unique<backtrackable_map_erase<rl_to_reps_t>>(rl));
    }
    rep_to_rls_.mutate(std::make_unique<backtrackable_map_erase<rep_to_rls_t>>(rep));
    return rls;
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename ILog>
std::unordered_set<uint32_t>
dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, ILog>::unlink(
    const resolution_lineage* rl) {
    if (!rl_to_reps_.get().contains(rl))
        return {};

    std::unordered_set<uint32_t> reps = rl_to_reps_.get().at(rl);
    for (auto rep : reps) {
        rep_to_rls_.mutate(std::make_unique<backtrackable_map_at_erase<rep_to_rls_t>>(rep, rl));
        if (rep_to_rls_.get().at(rep).empty())
            rep_to_rls_.mutate(std::make_unique<backtrackable_map_erase<rep_to_rls_t>>(rep));
    }
    rl_to_reps_.mutate(std::make_unique<backtrackable_map_erase<rl_to_reps_t>>(rl));
    return reps;
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename ILog>
void dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, ILog>::remove_head(
    const resolution_lineage* rl) {
    // Logical removal only: the arena slot is destroyed when its creation frame
    // pops, so the head object outlives this erase.
    if (heads_.get().contains(rl))
        heads_.mutate(std::make_unique<backtrackable_map_erase<heads_map_t>>(rl));
    unlink(rl);
}

#endif
