#include "../../hpp/infrastructure/multihead_unifier.hpp"
#include "../../hpp/domain/interfaces/i_factory.hpp"
#include "../../hpp/bootstrap/locator.hpp"

multihead_unifier::multihead_unifier() :
    db_(locator::locate<i_database>()),
    unifier_factory_(locator::locate<i_factory<i_unifier, std::unique_ptr<i_bind_map>>>()),
    overlay_bind_map_factory_(locator::locate<i_factory<i_overlay_bind_map, i_bind_map&>>()),
    common_(locator::locate<i_bind_map>()),
    translation_map_factory_(locator::locate<i_factory<i_translation_map>>()),
    copier_(locator::locate<i_copier>()) {
}

void multihead_unifier::add_head(const resolution_lineage* lineage) {
    
    // 1. get the rule from the db
    auto rule = db_.at(lineage->idx);
    // 2. create the translation map for copying
    auto tm = translation_map_factory_.make();
    // 3. copy the rule head
    auto copied_head = copier_.copy(rule.head, *tm);
    // 4. create the overlay bind map
    auto overlay_bind_map = overlay_bind_map_factory_.make(common_);
    // 5. get the parent goal's expr
    const expr* parent_goal_expr = frontier_.at(lineage->parent)->e;
    // 6. create the unifier
    auto unifier = unifier_factory_.make(std::move(overlay_bind_map));
    // 7. unify the parent goal's expr with the copied rule head
    unifier->unify(parent_goal_expr, copied_head);
    // 8. add the unifier to the map
    unifiers_.insert({lineage, std::move(unifier)});
}

void multihead_unifier::remove_head(const resolution_lineage* lineage) {
    // 1. remove the unifier from the map
    unifiers_.erase(lineage);
    // 2. unlink this lineage
    unlink(lineage);
}

void multihead_unifier::accept(const resolution_lineage* lineage) {
    
}

void multihead_unifier::reroot(uint32_t rep) {
    // 1. look up all rls for this rep
    auto rls = rep_to_rls_.at(rep);
    
    // 2. get new reps for this o.g. rep
    std::unordered_set<uint32_t> new_reps;
    extract_reps(rep, new_reps);

    // 3. unlink this rep from all heads
    auto unlinked_rls = unlink(rep);

    // 4. relink new reps to all heads
    link(new_reps, unlinked_rls);
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
    // 1. look up all heads for this rep
    auto rls = rep_to_rls_.at(rep);

    // 2. remove link from heads
    for (auto rl : rls) {
        auto& reps = rl_to_reps_.at(rl);
        reps.erase(rep);
        // 2.1 if no more reps, remove the entry
        if (reps.empty())
            rl_to_reps_.erase(rl);
    }

    // 3. remove the entry from rep_to_rls_
    rep_to_rls_.erase(rep);

    // 4. return the removed rls
    return rls;
}

std::unordered_set<uint32_t> multihead_unifier::unlink(const resolution_lineage* rl) {
    // 1. look up all reps for this head
    auto reps = rl_to_reps_.at(rl);
    
    // 2. remove link from reps
    for (auto rep : reps) {
        auto& heads = rep_to_rls_.at(rep);
        heads.erase(rl);
        // 2.1 if no more rls, remove the entry
        if (heads.empty())
            rep_to_rls_.erase(rep);
    }
    
    // 3. remove the entry from the head_to_reps_
    rl_to_reps_.erase(rl);

    // 4. return the removed reps
    return reps;
}
