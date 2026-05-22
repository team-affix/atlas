#include "../../hpp/infrastructure/mhu_elimination_generator.hpp"

mhu_elimination_generator::mhu_elimination_generator(
    i_bind_map& common_,
    i_lineage_pool& lp_,
    i_expr_pool& expr_pool_,
    i_bind_map_factory& bind_map_factory_,
    i_overlay_bind_map_factory& overlay_bind_map_factory_,
    i_unifier_factory& unifier_factory_,
    const i_get_goal_candidate_rules& get_goal_candidates_) :
    common_(common_),
    lp_(lp_),
    expr_pool_(expr_pool_),
    bind_map_factory_(bind_map_factory_),
    overlay_bind_map_factory_(overlay_bind_map_factory_),
    unifier_factory_(unifier_factory_),
    get_goal_candidates_(get_goal_candidates_) {
}

bool mhu_elimination_generator::try_add_head(const resolution_lineage* lineage, const expr* lhs, const expr* rhs) {
    // 1. construct a bind map
    auto bind_map = bind_map_factory_.make();
        
    // 2. construct an overlay bind map
    auto overlay_bind_map = overlay_bind_map_factory_.make(*bind_map, common_);

    // 3. construct a unifier on the overlay bind map
    auto unifier = unifier_factory_.make(*overlay_bind_map);

    // 4. unify and link
    if (!unify_and_link(*unifier, lineage, lhs, rhs))
        return false;

    // 5. construct a unify_head
    unify_head head{
        std::move(bind_map),
        std::move(overlay_bind_map),
        std::move(unifier)};
    
    // 6. add the head to the map
    heads_.insert({lineage, std::move(head)});

    return true;
}

state_machine<const resolution_lineage*> mhu_elimination_generator::constrain(const resolution_lineage* lineage) {
    // 1. get the parent goal lineage
    auto gl = lineage->parent;
    
    // 2. get all candidates in the family
    auto& candidates = get_goal_candidates_.get(gl);
    
    // 3. remove all siblings
    auto it_sm = candidates.iterate();
    while (!it_sm.done()) {
        auto candidate = it_sm.resume();
        if (!candidate.has_value()) continue;
        if (candidate.value() == lineage->idx) continue;
        remove_head(lp_.resolution(gl, candidate.value()));
    }
    
    // 4. get the head for this lineage
    auto& head = heads_.at(lineage);
    
    // 5. get the rep changes from the unifier
    auto rep_changes = unlink(lineage);
    
    // 6. for each rep change, add the link to new rep
    for (auto rep : rep_changes) {
        // 6.1 get new whnf
        auto new_rep = head.local_bind_map->whnf(expr_pool_.var(rep));
        // 6.2 bind the old rep to the new rep
        common_.bind(rep, new_rep);
        // 6.3 propagate the rep changes to all concerned heads
        auto sm0 = rebase(rep, new_rep);
        // 6.4 wait for the revalidation to complete
        while (!sm0.done()) {
            auto elim = sm0.resume();
            if (elim.has_value())
            co_yield elim.value();
        }
    }

    // 7. remove the head from the map
    heads_.erase(lineage);
}

state_machine<const resolution_lineage*> mhu_elimination_generator::rebase(uint32_t rep, const expr* new_rep) {
    // 1. unlink this rep from all heads
    auto remaining_rls = unlink(rep);

    // 2. for each rl, propagate the rep change to all heads
    for (auto rl : remaining_rls) {
        // 2.1 get the unifier for this rl
        auto& unifier = heads_.at(rl).unifier;
        
        // 2.1 unify and link the parent goal's expr with the copied rule head
        if (!unify_and_link(*unifier, rl, expr_pool_.var(rep), new_rep)) {
            remove_head(rl);
            co_yield rl;
        }
    }

    // NOTE: we actually want to leave the previously changed rep unlinked
    // from the heads since it is now up-to-date w.r.t. the common bind map
}

bool mhu_elimination_generator::unify_and_link(i_unifier& unifier, const resolution_lineage* lineage, const expr* lhs, const expr* rhs) {
    // 1. create rep_changes set
    std::unordered_set<uint32_t> rep_changes;
    // 2. unify the parent goal's expr with the copied rule head
    if (!unifier.unify(lhs, rhs, rep_changes))
        return false;
    // 3. link the new rl to all reps
    link(rep_changes, {lineage});
    // 4. return true
    return true;
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
