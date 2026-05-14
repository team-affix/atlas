#include "../../hpp/infrastructure/multihead_unifier.hpp"
#include "../../hpp/domain/interfaces/i_factory.hpp"
#include "../../hpp/bootstrap/locator.hpp"

multihead_unifier::multihead_unifier() :
    db_(locator::locate<i_database>()),
    unifier_factory_(locator::locate<i_factory<i_unifier, std::unique_ptr<i_bind_map>>>()),
    overlay_bind_map_factory_(locator::locate<i_factory<i_overlay_bind_map, i_bind_map&>>()),
    common_(locator::locate<i_bind_map>()),
    frontier_(locator::locate<i_frontier>()),
    translation_map_factory_(locator::locate<i_factory<i_translation_map>>()),
    copier_(locator::locate<i_copier>()),
    expr_pool_(locator::locate<i_expr_pool>()),
    rep_change_sink_factory_(locator::locate<i_factory<i_rep_change_sink>>()),
    head_unify_failed_producer_(locator::locate<i_event_producer<head_unify_failed_event>>()) {
}

void multihead_unifier::add_head(const resolution_lineage* lineage) {
    // 1. get the rule from the db
    auto rule = db_.at(lineage->idx);
    // 2. get the parent goal's expr
    const expr* parent_goal_expr = frontier_.at(lineage->parent)->e;
    // 3. get the copied rule head
    auto copied_head = frontier_.at(lineage->parent)->candidates.at(lineage->idx)->copied_head;
    // 4. create the overlay bind map
    auto overlay_bind_map = overlay_bind_map_factory_.make(common_);
    // 5. create the unifier
    auto unifier = unifier_factory_.make(std::move(overlay_bind_map));
    // 6. create rep_changes queue
    auto rep_change_sink = rep_change_sink_factory_.make();
    // 7. unify the parent goal's expr with the copied rule head
    unifier->unify(parent_goal_expr, copied_head, *rep_change_sink);
    // 8. add the unifier to the map
    heads_.insert({lineage, std::move(unifier)});
    // 9. link the new rl to all reps
    while (!rep_change_sink->empty())
        link({rep_change_sink->pop()}, {lineage});
}

void multihead_unifier::remove_head(const resolution_lineage* lineage) {
    // 1. remove the unifier from the map
    heads_.erase(lineage);
    // 2. unlink this lineage
    unlink(lineage);
}

void multihead_unifier::accept_head(const resolution_lineage* lineage) {
    // 1. get the unifier for this lineage
    auto& unifier = heads_.at(lineage);
    // 2. get the rep changes from the unifier
    auto rep_changes = rl_to_reps_.at(lineage);
    // 3. for each rep change, add the link to new rep
    for (auto rep : rep_changes) {
        // 3.1 get new whnf
        auto new_rep = unifier->bind_map->whnf(expr_pool_.var(rep));
        // 3.2 bind the old rep to the new rep
        common_.bind(rep, new_rep);
        // 3.3 propagate the rep changes to all concerned heads
        revalidate(rep, new_rep);
    }
}

void multihead_unifier::revalidate(uint32_t rep, const expr* new_rep) {
    // 1. unlink this rep from all heads
    auto invalidated_rls = unlink(rep);

    // 2. for each rl, propagate the rep change to all heads
    for (auto rl : invalidated_rls) {
        // 2.1 get the head
        auto& head = heads_.at(rl);
        // 2.2 construct the rep_change_sink
        auto rep_change_sink = rep_change_sink_factory_.make();
        // 2.3 unify the old rep with new rep
        bool success = head->unify(expr_pool_.var(rep), new_rep, *rep_change_sink);
        // 2.4 if failure, then this candidate is not applicable anymore
        if (!success) {
            remove_head(rl);
            head_unify_failed_producer_.produce(head_unify_failed_event{rl});
            continue;
        }
        // 2.5 if success, then link the lineage to all of the rep changes
        while (!rep_change_sink->empty())
            link({rep_change_sink->pop()}, {rl});
    }

    // 3. get new reps for this o.g. rep
    std::unordered_set<uint32_t> child_reps;
    extract_child_reps(expr_pool_.var(rep), child_reps);

    // 4. link child reps to all heads
    link(child_reps, invalidated_rls);
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
