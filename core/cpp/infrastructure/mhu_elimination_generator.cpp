#include "../../hpp/infrastructure/mhu_elimination_generator.hpp"

mhu_elimination_generator::mhu_elimination_generator(
    i_bind_map& common_,
    i_expr_pool& expr_pool_) :
    common_(common_),
    expr_pool_(expr_pool_) {
}

void mhu_elimination_generator::add_head(const resolution_lineage* lineage, unify_head head, const std::unordered_set<uint32_t>& rep_changes) {
    // 1. add the head to the map
    heads_.insert({lineage, std::move(head)});
    // 2. link the reps to the head
    link(rep_changes, {lineage});
}

void mhu_elimination_generator::try_remove_head(const resolution_lineage* lineage) {
    heads_.erase(lineage);
    unlink(lineage);
}

state_machine<const resolution_lineage*> mhu_elimination_generator::constrain(const resolution_lineage* lineage) {
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
            auto elim = sm0.resume();
            if (elim.has_value())
                co_yield elim.value();
        }
    }
    // 4. remove the head from the map
    heads_.erase(lineage);
}

state_machine<const resolution_lineage*> mhu_elimination_generator::revalidate(uint32_t rep, const expr* new_rep) {
    // 1. unlink this rep from all heads
    auto remaining_rls = unlink(rep);
    std::unordered_set<const resolution_lineage*> eliminated_rls;

    // 2. for each rl, propagate the rep change to all heads
    for (auto rl : remaining_rls) {
        // 2.1 unify and link the parent goal's expr with the copied rule head
        if (!unify_and_link(rl, expr_pool_.var(rep), new_rep)) {
            heads_.erase(rl);
            unlink(rl);
            eliminated_rls.insert(rl);
            co_yield rl;
        }
    }

    for (auto rl : eliminated_rls)
        remaining_rls.erase(rl);
}

bool mhu_elimination_generator::unify_and_link(const resolution_lineage* lineage, const expr* lhs, const expr* rhs) {
    // 1. get the head for this lineage
    auto& head = heads_.at(lineage);
    // 2. create rep_changes set
    std::unordered_set<uint32_t> rep_changes;
    // 3. unify the parent goal's expr with the copied rule head
    if (!head.unifier->unify(lhs, rhs, rep_changes))
        return false;
    // 4. link the new rl to all reps
    link(rep_changes, {lineage});
    // 5. return true
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
