#include <queue>
#include "infrastructure/mhu_elimination_generator.hpp"
#include "debug_assert.hpp"

mhu_elimination_generator::mhu_elimination_generator(locator& loc) :
    common_(loc.locate<i_bind_map>()),
    make_resolution_lineage_(loc.locate<i_make_resolution_lineage>()),
    make_var_(loc.locate<i_make_var>()),
    bind_map_factory_(loc.locate<i_bind_map_factory>()),
    unifier_factory_(loc.locate<i_unifier_factory>()),
    get_goal_candidate_rule_ids_(loc.locate<i_get_goal_candidate_rule_ids>()) {
}

bool mhu_elimination_generator::try_add_head(const resolution_lineage* lineage, const expr* lhs, const expr* rhs) {
    // 1. construct a bind map
    auto bind_map = bind_map_factory_.make();

    // 2. construct a unifier on the overlay bind map
    auto unifier = unifier_factory_.make(*bind_map);

    // 3. create the touched_vars queue
    std::queue<uint32_t> touched_vars;

    // 4. do the unification
    auto task = unifier->unify(lhs, rhs);

    // 5. wait for the unification to complete
    while (!task.done()) {
        task.resume();
        if (task.has_yield())
            touched_vars.push(task.consume_yield());
    }

    // 6. if the unification failed, return false
    if (!task.result())
        return false;
    
    // 7. sync and link
    if (!sync_and_link(lineage, *unifier, touched_vars))
        return false;

    // 8. construct a unify_head
    unify_head head{
        std::move(bind_map),
        std::move(unifier)};
    
    // 9. insert the head into the map
    const auto [_, inserted] = heads_.insert({lineage, std::move(head)});
    DEBUG_ASSERT(inserted);

    // 10. return true indicating the head was added
    return true;
}

coroutine<const resolution_lineage*, void> mhu_elimination_generator::constrain(const resolution_lineage* lineage) {
    // 1. get the parent goal lineage
    auto gl = lineage->parent;
    
    // 2. get all candidates in the family
    auto& candidates = get_goal_candidate_rule_ids_.get(gl);
    
    // 3. remove all siblings
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
    
    // 4. get the head for this lineage
    auto& head = heads_.at(lineage);
    
    // 5. get the rep changes from the unifier
    auto c_reps = unlink(lineage);
    
    // 6. accept the bindings
    auto sm0 = accept_bindings(*head.local_bind_map, c_reps);
    while (!sm0.done()) {
        sm0.resume();
        if (sm0.has_yield())
            co_yield sm0.consume_yield();
    }

    // 7. remove the head from the map
    heads_.erase(lineage);
}

void mhu_elimination_generator::clear_mhu_heads() {
    heads_.clear();
    rep_to_rls_.clear();
    rl_to_reps_.clear();
}

coroutine<const resolution_lineage*, void> mhu_elimination_generator::accept_bindings(i_bind_map& local_bind_map, const std::unordered_set<uint32_t>& c_reps) {
    for (auto c_rep : c_reps) {
        // 6.1. get the rep expr
        auto rep_expr = make_var_.make_var(c_rep);
        
        // 6.2. get new whnf
        auto whnf = local_bind_map.whnf(rep_expr);

        // 6.3. if the whnf is the same, then its still a c-rep, so skip
        if (rep_expr == whnf)
            continue;

        // 6.3 bind the old rep to the new rep
        common_.bind(c_rep, whnf);

        // 6.4 propagate the rep changes to all concerned heads
        auto sm0 = rebase_all(c_rep);

        // 6.5 wait for the revalidation to complete
        while (!sm0.done()) {
            sm0.resume();
            if (sm0.has_yield())
                co_yield sm0.consume_yield();
        }
    }
}

coroutine<const resolution_lineage*, void> mhu_elimination_generator::rebase_all(uint32_t rep) {
    // 1. unlink this rep from all heads
    auto remaining_rls = unlink(rep);

    // 2. for each rl, propagate the rep change to all heads
    for (auto rl : remaining_rls) {
        // 2.1. seed the touched_vars with the rep
        std::queue<uint32_t> touched_vars;
        touched_vars.push(rep);

        // 2.2. synchronize and link the touched vars
        if (!sync_and_link(rl, *heads_.at(rl).unifier, touched_vars)) {
            remove_head(rl);
            co_yield rl;
        }
    }

    // NOTE: we actually want to leave the previously changed rep unlinked
    // from the heads since it is now up-to-date w.r.t. the common bind map
}

bool mhu_elimination_generator::sync_and_link(const resolution_lineage* lineage, i_unifier& unifier, std::queue<uint32_t>& touched_vars) {
    // 1. create c_reps set
    std::unordered_set<uint32_t> c_reps;

    // 2. synchronize the touched vars, extracting the c_reps
    auto sync_task = synchronize(unifier, touched_vars);
    while (!sync_task.done()) {
        sync_task.resume();
        if (sync_task.has_yield())
            c_reps.insert(sync_task.consume_yield());
    }

    // 3. if the synchronization failed, return false
    if (!sync_task.result())
        return false;

    // 4. link the c_reps to the rl
    link(c_reps, {lineage});

    // 5. return true
    return true;
}

coroutine<uint32_t, bool> mhu_elimination_generator::synchronize(i_unifier& unifier, std::queue<uint32_t>& touched_vars) {
    // 1. process work queue
    while (!touched_vars.empty()) {
        // 1.1. get the rep
        auto var = touched_vars.front();
        touched_vars.pop();

        // 1.2. get the rep expr
        auto var_expr = make_var_.make_var(var);

        // 1.3. get the whnf of the rep
        auto whnf = common_.whnf(var_expr);

        // 1.4. if the rep is a c-rep (common rep), yield it
        if (var_expr == whnf) {
            co_yield var;
            continue;
        }

        // 1.5. if the rep is NOT a c-rep, unify
        auto task = unifier.unify(var_expr, whnf);

        // 1.6. wait for the unification to complete
        while (!task.done()) {
            task.resume();
            if (task.has_yield())
                touched_vars.push(task.consume_yield());
        }

        // 1.7. if the unification failed, return false
        if (!task.result())
            co_return false;
    }

    // 2. return true
    co_return true;
}

void mhu_elimination_generator::link(const std::unordered_set<uint32_t>& reps, const std::unordered_set<const resolution_lineage*>& rls) {
    // 1. for each rep, add the link to the map
    for (auto rep : reps) {
        rep_to_rls_[rep].insert(rls.begin(), rls.end());
    }
    // 2. for each rl, add the link to the map
    for (auto rl : rls) {
        rl_to_reps_[rl].insert(reps.begin(), reps.end());
    }
}

std::unordered_set<const resolution_lineage*> mhu_elimination_generator::unlink(uint32_t rep) {
    // 1. extract the entry, removing it from the map
    auto node = rep_to_rls_.extract(rep);
    if (node.empty())
        return {};

    // 2. get the rls
    auto& rls = node.mapped();

    // 3. remove link from heads
    for (auto rl : rls) {
        auto& reps = rl_to_reps_.at(rl);
        reps.erase(rep);
        // 3.1 if no more reps, remove the entry
        if (reps.empty())
            rl_to_reps_.erase(rl);
    }

    // 4. return the removed rls
    return std::move(rls);
}

std::unordered_set<uint32_t> mhu_elimination_generator::unlink(const resolution_lineage* rl) {
    // 1. extract the entry, removing it from the map
    auto node = rl_to_reps_.extract(rl);
    if (node.empty())
        return {};

    // 2. get the reps
    auto& reps = node.mapped();
    
    // 3. remove link from reps
    for (auto rep : reps) {
        auto& heads = rep_to_rls_.at(rep);
        heads.erase(rl);
        // 3.1 if no more rls, remove the entry
        if (heads.empty())
            rep_to_rls_.erase(rep);
    }
    
    // 4. return the removed reps
    return std::move(reps);
}

void mhu_elimination_generator::remove_head(const resolution_lineage* rl) {
    heads_.erase(rl);
    unlink(rl);
}
