#ifndef DBUCT_MHU_ELIMINATION_GENERATOR_HPP
#define DBUCT_MHU_ELIMINATION_GENERATOR_HPP

#include <cstdint>
#include <deque>
#include <list>
#include <memory>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include "infrastructure/bind_map_trail.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/frame_savepoint.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/mhu_action.hpp"
#include "value_objects/unify_head.hpp"
#include "debug_assert.hpp"

template<typename IBindMap, typename IBindMapFactory, typename IUnifier,
         typename IUnifierFactory, typename IMakeResolutionLineage,
         typename IMakeVar, typename IGetGoalCandidateRuleIds, typename IFrameHub>
struct dbuct_mhu_elimination_generator {
    dbuct_mhu_elimination_generator(IBindMap&, IMakeResolutionLineage&, IMakeVar&,
                                    IBindMapFactory&, IUnifierFactory&,
                                    const IGetGoalCandidateRuleIds&);

    void bind_hub(IFrameHub&);

    bool try_add_head(const resolution_lineage*, framed_expr goal, framed_expr head);
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
    void remove_head(const resolution_lineage*);

    void push_frame();
    void pop_frame();
    void squash_frame();

private:
    using head_t       = unify_head<IBindMap, IUnifier>;
    using arena_t      = std::deque<head_t>;
    using heads_map_t  = std::unordered_map<const resolution_lineage*, head_t*>;
    using rep_to_rls_t = std::unordered_map<uint32_t, std::unordered_set<const resolution_lineage*>>;
    using rl_to_reps_t = std::unordered_map<const resolution_lineage*, std::unordered_set<uint32_t>>;
    using action_t     = mhu_action<IBindMap, IUnifier>;

    struct frame {
        std::list<action_t> actions;
    };

    coroutine<const resolution_lineage*, void> accept_bindings(IBindMap&, const std::unordered_set<uint32_t>&);
    coroutine<const resolution_lineage*, void> rebase_all(uint32_t);
    bool sync_and_link(const resolution_lineage*, IUnifier&, std::queue<uint32_t>&);
    coroutine<uint32_t, bool> synchronize(IUnifier&, std::queue<uint32_t>&);
    void link(const std::unordered_set<uint32_t>&, const std::unordered_set<const resolution_lineage*>&);
    std::unordered_set<const resolution_lineage*> unlink(uint32_t);
    std::unordered_set<uint32_t> unlink(const resolution_lineage*);

    void log(action_t action);
    void undo_action(const action_t& action);

    IBindMap& common_;
    IMakeResolutionLineage& make_resolution_lineage_;
    IMakeVar& make_var_;
    IBindMapFactory& bind_map_factory_;
    IUnifierFactory& unifier_factory_;
    const IGetGoalCandidateRuleIds& get_goal_candidate_rule_ids_;
    IFrameHub* hub_ = nullptr;

    arena_t arena_;
    heads_map_t heads_;
    rep_to_rls_t rep_to_rls_;
    rl_to_reps_t rl_to_reps_;
    std::stack<frame> frame_stack_;
    bind_map_trail<IBindMap> trail_;
};

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::dbuct_mhu_elimination_generator(
    IBM& common, IMRL& mrl, IMV& mv, IBMF& bmf, IUF& uf, const IGCRI& gcri)
    : common_(common), make_resolution_lineage_(mrl), make_var_(mv),
      bind_map_factory_(bmf), unifier_factory_(uf),
      get_goal_candidate_rule_ids_(gcri) {}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
void dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::bind_hub(IHUB& hub) {
    hub_ = &hub;
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
bool dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::try_add_head(
    const resolution_lineage* lineage, framed_expr lhs, framed_expr rhs) {
    frame_savepoint savepoint(*hub_);

    auto bm = std::make_unique<IBM>(bind_map_factory_.make());
    IBM* const local_bind_map = bm.get();
    local_bind_map->attach_trail(&trail_);
    IU u = unifier_factory_.make(local_bind_map);
    arena_.emplace_back(std::move(bm), std::move(u));
    log(mhu_arena_emplace{});
    head_t* head_ptr = &arena_.back();

    std::queue<uint32_t> touched_vars;
    auto task = head_ptr->unifier.unify(lhs, rhs);
    while (!task.done()) {
        task.resume();
        if (task.has_yield())
            touched_vars.push(task.consume_yield());
    }

    if (!task.result())
        return false;

    if (!sync_and_link(lineage, head_ptr->unifier, touched_vars))
        return false;

    heads_.insert({lineage, head_ptr});
    log(mhu_heads_insert<IBM, IU>{lineage, head_ptr});
    savepoint.commit();
    return true;
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
coroutine<const resolution_lineage*, void>
dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::constrain(
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

    head_t* head = heads_.at(lineage);
    auto c_reps = unlink(lineage);
    auto sm0 = accept_bindings(*head->local_bind_map, c_reps);
    while (!sm0.done()) {
        sm0.resume();
        if (sm0.has_yield())
            co_yield sm0.consume_yield();
    }

    if (heads_.contains(lineage)) {
        head_t* captured = heads_.at(lineage);
        heads_.erase(lineage);
        log(mhu_heads_erase<IBM, IU>{lineage, captured});
    }
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
coroutine<const resolution_lineage*, void>
dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::accept_bindings(
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

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
coroutine<const resolution_lineage*, void>
dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::rebase_all(uint32_t rep) {
    auto remaining_rls = unlink(rep);
    for (auto rl : remaining_rls) {
        std::queue<uint32_t> touched_vars;
        touched_vars.push(rep);
        if (!sync_and_link(rl, heads_.at(rl)->unifier, touched_vars)) {
            remove_head(rl);
            co_yield rl;
        }
    }
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
bool dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::sync_and_link(
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

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
coroutine<uint32_t, bool>
dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::synchronize(
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

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
void dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::link(
    const std::unordered_set<uint32_t>& reps,
    const std::unordered_set<const resolution_lineage*>& rls) {
    for (auto rep : reps) {
        if (!rep_to_rls_.contains(rep)) {
            rep_to_rls_.insert({rep, std::unordered_set<const resolution_lineage*>{}});
            log(mhu_rep_map_insert{rep, {}});
        }
        for (auto rl : rls) {
            if (!rep_to_rls_.at(rep).contains(rl)) {
                rep_to_rls_.at(rep).insert(rl);
                log(mhu_rep_at_insert{rep, rl});
            }
        }
    }
    for (auto rl : rls) {
        if (!rl_to_reps_.contains(rl)) {
            rl_to_reps_.insert({rl, std::unordered_set<uint32_t>{}});
            log(mhu_rl_map_insert{rl, {}});
        }
        for (auto rep : reps) {
            if (!rl_to_reps_.at(rl).contains(rep)) {
                rl_to_reps_.at(rl).insert(rep);
                log(mhu_rl_at_insert{rl, rep});
            }
        }
    }
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
std::unordered_set<const resolution_lineage*>
dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::unlink(uint32_t rep) {
    if (!rep_to_rls_.contains(rep))
        return {};

    std::unordered_set<const resolution_lineage*> rls = rep_to_rls_.at(rep);
    for (auto rl : rls) {
        rl_to_reps_.at(rl).erase(rep);
        log(mhu_rl_at_erase{rl, rep});
        if (rl_to_reps_.at(rl).empty()) {
            std::unordered_set<uint32_t> captured = std::move(rl_to_reps_.at(rl));
            rl_to_reps_.erase(rl);
            log(mhu_rl_map_erase{rl, std::move(captured)});
        }
    }
    std::unordered_set<const resolution_lineage*> captured = std::move(rep_to_rls_.at(rep));
    rep_to_rls_.erase(rep);
    log(mhu_rep_map_erase{rep, std::move(captured)});
    return rls;
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
std::unordered_set<uint32_t>
dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::unlink(
    const resolution_lineage* rl) {
    if (!rl_to_reps_.contains(rl))
        return {};

    std::unordered_set<uint32_t> reps = rl_to_reps_.at(rl);
    for (auto rep : reps) {
        rep_to_rls_.at(rep).erase(rl);
        log(mhu_rep_at_erase{rep, rl});
        if (rep_to_rls_.at(rep).empty()) {
            std::unordered_set<const resolution_lineage*> captured = std::move(rep_to_rls_.at(rep));
            rep_to_rls_.erase(rep);
            log(mhu_rep_map_erase{rep, std::move(captured)});
        }
    }
    std::unordered_set<uint32_t> captured = std::move(rl_to_reps_.at(rl));
    rl_to_reps_.erase(rl);
    log(mhu_rl_map_erase{rl, std::move(captured)});
    return reps;
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
void dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::remove_head(
    const resolution_lineage* rl) {
    if (!heads_.contains(rl))
        return;
    head_t* captured = heads_.at(rl);
    heads_.erase(rl);
    log(mhu_heads_erase<IBM, IU>{rl, captured});
    unlink(rl);
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
void dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::push_frame() {
    trail_.push_frame();
    frame_stack_.push(frame{});
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
void dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::pop_frame() {
    trail_.pop_frame();
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
void dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::squash_frame() {
    trail_.squash_frame();
    auto top = std::move(frame_stack_.top());
    frame_stack_.pop();
    auto& parent = frame_stack_.top().actions;
    parent.splice(parent.end(), std::move(top.actions));
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
void dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::log(action_t action) {
    DEBUG_ASSERT(!frame_stack_.empty());
    frame_stack_.top().actions.push_back(std::move(action));
}

template<typename IBM, typename IBMF, typename IU, typename IUF, typename IMRL, typename IMV, typename IGCRI, typename IHUB>
void dbuct_mhu_elimination_generator<IBM, IBMF, IU, IUF, IMRL, IMV, IGCRI, IHUB>::undo_action(
    const action_t& action) {
    std::visit([&](const auto& op) {
        using T = std::decay_t<decltype(op)>;
        if constexpr (std::is_same_v<T, mhu_arena_emplace>) {
            arena_.pop_back();
        } else if constexpr (std::is_same_v<T, mhu_heads_insert<IBM, IU>>) {
            heads_.erase(op.rl);
        } else if constexpr (std::is_same_v<T, mhu_heads_erase<IBM, IU>>) {
            heads_.insert({op.rl, op.head});
        } else if constexpr (std::is_same_v<T, mhu_rep_map_insert>) {
            rep_to_rls_.erase(op.rep);
        } else if constexpr (std::is_same_v<T, mhu_rep_map_erase>) {
            rep_to_rls_.insert({op.rep, op.value});
        } else if constexpr (std::is_same_v<T, mhu_rep_at_insert>) {
            rep_to_rls_.at(op.rep).erase(op.rl);
        } else if constexpr (std::is_same_v<T, mhu_rep_at_erase>) {
            rep_to_rls_.at(op.rep).insert(op.rl);
        } else if constexpr (std::is_same_v<T, mhu_rl_map_insert>) {
            rl_to_reps_.erase(op.rl);
        } else if constexpr (std::is_same_v<T, mhu_rl_map_erase>) {
            rl_to_reps_.insert({op.rl, op.value});
        } else if constexpr (std::is_same_v<T, mhu_rl_at_insert>) {
            rl_to_reps_.at(op.rl).erase(op.rep);
        } else if constexpr (std::is_same_v<T, mhu_rl_at_erase>) {
            rl_to_reps_.at(op.rl).insert(op.rep);
        }
    }, action);
}

#endif
