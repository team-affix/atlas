#ifndef DBUCT_MHU_ELIMINATION_GENERATOR_HPP
#define DBUCT_MHU_ELIMINATION_GENERATOR_HPP

#include <cstdint>
#include <deque>
#include <list>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include "infrastructure/coroutine.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/mhu_action.hpp"
#include "value_objects/unify_head.hpp"
#include "debug_assert.hpp"

template<typename IBindMap, typename IBindCommonVar, typename IWhnfCommonExpr,
         typename IAcquireBindMap, typename IReleaseBindMap, typename IEmplaceBindMap,
         typename IMakeBindMap, typename IUnifier, typename IUnifierFactory,
         typename IMakeResolutionLineage, typename IMakeVar, typename IGetGoalCandidateRuleIds>
struct dbuct_mhu_elimination_generator {
    dbuct_mhu_elimination_generator(IBindCommonVar&, IWhnfCommonExpr&, IMakeResolutionLineage&, IMakeVar&,
                                    IAcquireBindMap&, IReleaseBindMap&, IEmplaceBindMap&, IMakeBindMap&,
                                    IUnifierFactory&, const IGetGoalCandidateRuleIds&);

    bool try_add_head(const resolution_lineage*, framed_expr goal, framed_expr head);
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
    void remove_head(const resolution_lineage*);

    void push_frame();
    void pop_frame();

private:
    using head_t  = unify_head<IBindMap, IUnifier>;
    using vec_t   = std::deque<head_t>;
    using map_t   = std::unordered_map<const resolution_lineage*, head_t*>;
    using rep_map_t = std::unordered_map<uint32_t, std::unordered_set<const resolution_lineage*>>;
    using rl_map_t  = std::unordered_map<const resolution_lineage*, std::unordered_set<uint32_t>>;
    using action_t  = mhu_action<IBindMap, IUnifier>;

    struct frame {
        std::list<action_t> actions_;
        std::unordered_set<IBindMap*> visited;
    };

    coroutine<const resolution_lineage*, void> accept_bindings(IBindMap&, const std::unordered_set<uint32_t>&);
    coroutine<const resolution_lineage*, void> rebase_all(uint32_t);
    bool sync_and_link(const resolution_lineage*, IUnifier&, std::queue<uint32_t>&);
    coroutine<uint32_t, bool> synchronize(IUnifier&, std::queue<uint32_t>&);
    void link(const std::unordered_set<uint32_t>&, const std::unordered_set<const resolution_lineage*>&);
    std::unordered_set<const resolution_lineage*> unlink_rep(uint32_t);
    std::unordered_set<uint32_t> unlink_resolution(const resolution_lineage*);

    void log(action_t action);
    void undo_action(const action_t& action);
    void visit(IBindMap* m);

    IBindCommonVar& bind_common_var_;
    IWhnfCommonExpr& whnf_common_expr_;
    IMakeResolutionLineage& make_resolution_lineage_;
    IMakeVar& make_var_;
    IAcquireBindMap& acquire_bind_map_;
    IReleaseBindMap& release_bind_map_;
    IEmplaceBindMap& emplace_bind_map_;
    IMakeBindMap& make_bind_map_;
    IUnifierFactory& unifier_factory_;
    const IGetGoalCandidateRuleIds& get_goal_candidate_rule_ids_;

    vec_t vec_;
    map_t map_;
    rep_map_t rep_map_;
    rl_map_t rl_map_;
    std::stack<frame> frame_stack_;
};

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::dbuct_mhu_elimination_generator(
    IBCV& bcv, IWCE& wce, IMRL& mrl, IMV& mv, IABM& abm, IRBM& rbm, IEBM& ebm, IMBM& mbm,
    IUF& uf, const IGCRI& gcri)
    : bind_common_var_(bcv), whnf_common_expr_(wce), make_resolution_lineage_(mrl), make_var_(mv),
      acquire_bind_map_(abm), release_bind_map_(rbm), emplace_bind_map_(ebm), make_bind_map_(mbm),
      unifier_factory_(uf), get_goal_candidate_rule_ids_(gcri), frame_stack_(std::deque<frame>{frame{}}) {}

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
bool dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::try_add_head(
    const resolution_lineage* lineage, framed_expr lhs, framed_expr rhs) {
    IBM* bm = acquire_bind_map_.acquire();
    if (!bm)
        bm = emplace_bind_map_.emplace(make_bind_map_.make());
    else
        bm->clear_bindings();
    IU u = unifier_factory_.make(bm);

    std::queue<uint32_t> touched_vars;
    auto task = u.unify(lhs, rhs);
    while (!task.done()) {
        task.resume();
        if (task.has_yield())
            touched_vars.push(task.consume_yield());
    }
    if (!task.result()) {
        release_bind_map_.release(bm);
        return false;
    }

    if (!sync_and_link(lineage, u, touched_vars)) {
        release_bind_map_.release(bm);
        return false;
    }

    vec_.emplace_back(head_t{bm, std::move(u)});
    log(mhu_arena_emplace{});
    head_t* head_ptr = &vec_.back();
    map_.insert({lineage, head_ptr});
    log(mhu_heads_insert<IBM, IU>{lineage, head_ptr});
    return true;
}

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
coroutine<const resolution_lineage*, void>
dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::constrain(
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

    head_t* head = map_.at(lineage);
    auto c_reps = unlink_resolution(lineage);
    auto sm0 = accept_bindings(*head->local_bind_map, c_reps);
    while (!sm0.done()) {
        sm0.resume();
        if (sm0.has_yield())
            co_yield sm0.consume_yield();
    }

    if (map_.contains(lineage)) {
        head_t* captured = map_.at(lineage);
        map_.erase(lineage);
        log(mhu_heads_erase<IBM, IU>{lineage, captured});
    }
}

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
coroutine<const resolution_lineage*, void>
dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::accept_bindings(
    IBM& local_bind_map, const std::unordered_set<uint32_t>& c_reps) {
    for (auto c_rep : c_reps) {
        visit(&local_bind_map);
        auto rep_expr = make_var_.make_var(c_rep);
        framed_expr whnf = local_bind_map.whnf({rep_expr, 0});

        if (rep_expr == whnf.skeleton && whnf.frame_offset == 0)
            continue;

        bind_common_var_.bind(c_rep, whnf);

        auto sm0 = rebase_all(c_rep);
        while (!sm0.done()) {
            sm0.resume();
            if (sm0.has_yield())
                co_yield sm0.consume_yield();
        }
    }
}

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
coroutine<const resolution_lineage*, void>
dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::rebase_all(uint32_t rep) {
    auto remaining_rls = unlink_rep(rep);
    for (auto rl : remaining_rls) {
        std::queue<uint32_t> touched_vars;
        touched_vars.push(rep);
        visit(map_.at(rl)->local_bind_map);
        if (!sync_and_link(rl, map_.at(rl)->unifier, touched_vars)) {
            remove_head(rl);
            co_yield rl;
        }
    }
}

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
bool dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::sync_and_link(
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

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
coroutine<uint32_t, bool>
dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::synchronize(
    IU& unifier, std::queue<uint32_t>& touched_vars) {
    while (!touched_vars.empty()) {
        auto var = touched_vars.front();
        touched_vars.pop();

        auto var_expr = make_var_.make_var(var);
        framed_expr whnf = whnf_common_expr_.whnf({var_expr, 0});

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

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
void dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::link(
    const std::unordered_set<uint32_t>& reps,
    const std::unordered_set<const resolution_lineage*>& rls) {
    for (auto rep : reps) {
        if (!rep_map_.contains(rep)) {
            rep_map_.insert({rep, std::unordered_set<const resolution_lineage*>{}});
            log(mhu_rep_map_insert{rep, {}});
        }
        for (auto rl : rls) {
            if (!rep_map_.at(rep).contains(rl)) {
                rep_map_.at(rep).insert(rl);
                log(mhu_rep_at_insert{rep, rl});
            }
        }
    }
    for (auto rl : rls) {
        if (!rl_map_.contains(rl)) {
            rl_map_.insert({rl, std::unordered_set<uint32_t>{}});
            log(mhu_rl_map_insert{rl, {}});
        }
        for (auto rep : reps) {
            if (!rl_map_.at(rl).contains(rep)) {
                rl_map_.at(rl).insert(rep);
                log(mhu_rl_at_insert{rl, rep});
            }
        }
    }
}

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
std::unordered_set<const resolution_lineage*>
dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::unlink_rep(uint32_t rep) {
    if (!rep_map_.contains(rep))
        return {};

    std::unordered_set<const resolution_lineage*> rls = rep_map_.at(rep);
    for (auto rl : rls) {
        rl_map_.at(rl).erase(rep);
        log(mhu_rl_at_erase{rl, rep});
        if (rl_map_.at(rl).empty()) {
            std::unordered_set<uint32_t> captured = std::move(rl_map_.at(rl));
            rl_map_.erase(rl);
            log(mhu_rl_map_erase{rl, std::move(captured)});
        }
    }
    std::unordered_set<const resolution_lineage*> captured = std::move(rep_map_.at(rep));
    rep_map_.erase(rep);
    log(mhu_rep_map_erase{rep, std::move(captured)});
    return rls;
}

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
std::unordered_set<uint32_t>
dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::unlink_resolution(
    const resolution_lineage* rl) {
    if (!rl_map_.contains(rl))
        return {};

    std::unordered_set<uint32_t> reps = rl_map_.at(rl);
    for (auto rep : reps) {
        rep_map_.at(rep).erase(rl);
        log(mhu_rep_at_erase{rep, rl});
        if (rep_map_.at(rep).empty()) {
            std::unordered_set<const resolution_lineage*> captured = std::move(rep_map_.at(rep));
            rep_map_.erase(rep);
            log(mhu_rep_map_erase{rep, std::move(captured)});
        }
    }
    std::unordered_set<uint32_t> captured = std::move(rl_map_.at(rl));
    rl_map_.erase(rl);
    log(mhu_rl_map_erase{rl, std::move(captured)});
    return reps;
}

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
void dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::remove_head(
    const resolution_lineage* rl) {
    if (!map_.contains(rl))
        return;
    head_t* captured = map_.at(rl);
    map_.erase(rl);
    log(mhu_heads_erase<IBM, IU>{rl, captured});
    unlink_resolution(rl);
}

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
void dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::push_frame() {
    frame_stack_.push(frame{});
}

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
void dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (IBM* m : current.visited)
        m->pop_frame();
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
}

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
void dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::log(action_t action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions_.push_back(std::move(action));
}

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
void dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::undo_action(
    const action_t& action) {
    if (std::holds_alternative<mhu_arena_emplace>(action)) {
        release_bind_map_.release(vec_.back().local_bind_map);
        vec_.pop_back();
    } else if (const auto* op = std::get_if<mhu_heads_insert<IBM, IU>>(&action)) {
        map_.erase(op->rl);
    } else if (const auto* op = std::get_if<mhu_heads_erase<IBM, IU>>(&action)) {
        map_.insert({op->rl, op->head});
    } else if (const auto* op = std::get_if<mhu_rep_map_insert>(&action)) {
        rep_map_.erase(op->rep);
    } else if (const auto* op = std::get_if<mhu_rep_map_erase>(&action)) {
        rep_map_.insert({op->rep, op->value});
    } else if (const auto* op = std::get_if<mhu_rep_at_insert>(&action)) {
        rep_map_.at(op->rep).erase(op->rl);
    } else if (const auto* op = std::get_if<mhu_rep_at_erase>(&action)) {
        rep_map_.at(op->rep).insert(op->rl);
    } else if (const auto* op = std::get_if<mhu_rl_map_insert>(&action)) {
        rl_map_.erase(op->rl);
    } else if (const auto* op = std::get_if<mhu_rl_map_erase>(&action)) {
        rl_map_.insert({op->rl, op->value});
    } else if (const auto* op = std::get_if<mhu_rl_at_insert>(&action)) {
        rl_map_.at(op->rl).erase(op->rep);
    } else if (const auto* op = std::get_if<mhu_rl_at_erase>(&action)) {
        rl_map_.at(op->rl).insert(op->rep);
    }
}

template<typename IBM, typename IBCV, typename IWCE, typename IABM, typename IRBM, typename IEBM,
         typename IMBM, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI>
void dbuct_mhu_elimination_generator<IBM, IBCV, IWCE, IABM, IRBM, IEBM, IMBM, IU, IUF, IMRL, IMV, IGCRI>::visit(IBM* m) {
    if (frame_stack_.top().visited.insert(m).second)
        m->push_frame();
}

#endif
