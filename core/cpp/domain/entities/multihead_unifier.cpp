#include "../../../hpp/domain/entities/multihead_unifier.hpp"
#include "../../../hpp/domain/interfaces/i_factory.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

multihead_unifier::multihead_unifier() :
    db_(locator::locate<i_database>()),
    unifier_factory_(locator::locate<i_factory<i_unifier, i_bind_map&>>()),
    bind_map_factory_(locator::locate<i_factory<i_bind_map>>()),
    overlay_bind_map_factory_(locator::locate<i_factory<i_overlay_bind_map, i_bind_map&, i_bind_map&>>()),
    common_(locator::locate<i_bind_map>()),
    frontier_(locator::locate<i_frontier>()),
    copier_(locator::locate<i_copier>()),
    expr_pool_(locator::locate<i_expr_pool>()),
    multihead_unify_accept_yielded_producer_(locator::locate<i_event_producer<multihead_unify_accept_yielded_event>>()) {
}

void multihead_unifier::add_head(const resolution_lineage* lineage) {
    // 1. get the rule from the db
    auto rule = db_.at(lineage->idx);
    // 2. get the parent goal's expr
    const expr* parent_goal_expr = frontier_.at(lineage->parent)->e;
    // 3. get the copied rule head
    auto copied_head = frontier_.at(lineage->parent)->candidates.at(lineage->idx)->copied_head;
    // 4. create the local bind map
    auto local_bind_map = bind_map_factory_.make();
    // 5. create the overlay bind map
    auto overlay_bind_map = overlay_bind_map_factory_.make(*local_bind_map, common_);
    // 6. create the unifier
    auto unifier = unifier_factory_.make(*overlay_bind_map);
    // 7. add the unifier to the map
    heads_.insert({lineage, unify_head{
        std::move(local_bind_map),
        std::move(overlay_bind_map),
        std::move(unifier)}});
    // 8. unify and link the parent goal's expr with the copied rule head
    unify_and_link(lineage, parent_goal_expr, copied_head);
}

void multihead_unifier::remove_head(const resolution_lineage* lineage) {
    // 1. remove the unifier from the map
    heads_.erase(lineage);
    // 2. unlink this lineage
    unlink(lineage);
}

void multihead_unifier::init_accept_head(const resolution_lineage* lineage) {
    accept_head_state_machine = accept_head(lineage);
    // yield to start the state machine
    multihead_unify_accept_yielded_producer_.produce(multihead_unify_accept_yielded_event{});
}

void multihead_unifier::resume_accept_head() {
    accept_head_state_machine->resume();
    if (!accept_head_state_machine->done()) {
        // emit yield event
        multihead_unify_accept_yielded_producer_.produce(multihead_unify_accept_yielded_event{});
    }
}

void multihead_unifier::unify_and_link(const resolution_lineage* lineage, const expr* lhs, const expr* rhs) {
    // 1. get the head for this lineage
    auto& head = heads_.at(lineage);
    // 2. create rep_changes set
    std::unordered_set<uint32_t> rep_changes;
    // 3. unify the parent goal's expr with the copied rule head
    bool success = head.unifier->unify(lhs, rhs, rep_changes);
    // 4. if failure, remove the head
    if (!success) {
        remove_head(lineage);
        frontier_.eliminate(lineage);
        return;
    }
    // 5. link the new rl to all reps
    link(rep_changes, {lineage});
}

void multihead_unifier::link(const std::unordered_set<uint32_t>& reps, const std::unordered_set<const resolution_lineage*>& rls) {
    // 1. for each rep, add the link to the map
    for (auto rep : reps) {
        rep_to_rls_[rep].insert(rls.begin(), rls.end());
    }
    // 2. for each rl, add the link to the map
    for (auto rl : rls) {
        rl_to_reps_[rl].insert(reps.begin(), reps.end());
    }
}

std::unordered_set<const resolution_lineage*> multihead_unifier::unlink(uint32_t rep) {
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

std::unordered_set<uint32_t> multihead_unifier::unlink(const resolution_lineage* rl) {
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

void multihead_unifier::extract_child_reps(const expr* e, std::unordered_set<uint32_t>& child_reps) {
    // 1. get whnf of the expr
    auto whnf = common_.whnf(e);
    // 2. if whnf is a variable, add it to the new reps
    if (const expr::var* v = std::get_if<expr::var>(&whnf->content)) {
        child_reps.insert(v->index);
        return;
    }
    // 3. whnf is a functor
    const expr::functor& f = std::get<expr::functor>(whnf->content);
    // 4. for each argument, recur
    for (auto& arg : f.args) {
        extract_child_reps(arg, child_reps);
    }
}

state_machine<void> multihead_unifier::accept_head(const resolution_lineage* lineage) {
    // 1. get the unifier for this lineage
    auto& head = heads_.at(lineage);
    // 2. get the rep changes from the unifier
    auto rep_changes = unlink(lineage);
    // 3. for each rep change, add the link to new rep
    for (auto rep : rep_changes) {
        // 3.1 get new whnf
        auto new_rep = head.local_bind_map->whnf(expr_pool_.var(rep));
        // 3.2 bind the old rep to the new rep
        common_.bind(rep, new_rep);
        // 3.3 propagate the rep changes to all concerned heads
        auto sm0 = revalidate(rep, new_rep);
        // 3.4 wait for the revalidation to complete
        while (!sm0.done()) {
            sm0.resume();
            co_await std::suspend_always{};
        }
    }
    // 4. remove the head from the map
    heads_.erase(lineage);
}

state_machine<void> multihead_unifier::revalidate(uint32_t rep, const expr* new_rep) {
    // 1. unlink this rep from all heads
    auto invalidated_rls = unlink(rep);

    // 2. for each rl, propagate the rep change to all heads
    for (auto rl : invalidated_rls) {
        // 2.1 unify and link the parent goal's expr with the copied rule head
        unify_and_link(rl, expr_pool_.var(rep), new_rep);
        co_await std::suspend_always{};
    }

    // 3. get new reps for this o.g. rep
    std::unordered_set<uint32_t> child_reps;
    extract_child_reps(expr_pool_.var(rep), child_reps);

    // 4. link child reps to all heads
    link(child_reps, invalidated_rls);
}
