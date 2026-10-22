#include "infrastructure/pud_runtime.hpp"

#include <algorithm>
#include <unordered_set>
#include "debug_assert.hpp"

pud_runtime::pud_runtime(db& database)
    : database_(database)
    , database_opened_(false)
    , manifest_() {}

pud_frontier_change pud_runtime::open_database() {
    DEBUG_ASSERT(!database_opened_);
    database_opened_ = true;
    manifest_.frontier_recorder_.clear();

    std::vector<rule_id> rule_ids;
    auto rule_id_iterator = database_.lookup_all_rules().iterate();
    while (auto id = rule_id_iterator.next())
        rule_ids.push_back(*id);
    std::sort(rule_ids.begin(), rule_ids.end());

    std::vector<pud_node_id> roots;
    for (rule_id id : rule_ids)
        roots.push_back(manifest_.axiom_initializer_.initialize_axiom(*database_.get_rule(id)));
    for (pud_node_id root : roots)
        manifest_.base_descent_runner_.run(root);
    return collect_change(roots);
}

pud_frontier_change pud_runtime::add_conjecture(
    const expr* head, const initial_goal_exprs& goals, uint32_t var_count) {
    DEBUG_ASSERT(database_opened_);
    manifest_.frontier_recorder_.clear();

    std::vector<const expr*> goal_exprs;
    for (size_t index = 0; index < goals.count(); ++index)
        goal_exprs.push_back(goals.get(index));
    const pud_node_id root = manifest_.conjecture_initializer_.initialize_conjecture(head, goal_exprs, var_count);
    manifest_.base_descent_runner_.run(root);
    return collect_change({root});
}

pud_frontier_change pud_runtime::expand(pud_derivation_id derivation, pud_body_goal_id goal) {
    const pud_node_id leaf = derivation.value;
    DEBUG_ASSERT(manifest_.open_leaves_.check_leaf_open(leaf));
    DEBUG_ASSERT(manifest_.node_descents_.get(leaf).body_goal_pq().find_query_head(goal).has_value());
    manifest_.frontier_recorder_.clear();
    manifest_.unfolder_.unfold(leaf, goal);
    return collect_change(manifest_.children_.get(leaf));
}

const expr* pud_runtime::head(pud_derivation_id derivation) {
    DEBUG_ASSERT(manifest_.node_descents_.check_descent(derivation.value));
    return manifest_.node_heads_.get(find_root(derivation.value));
}

std::vector<pud_body_goal> pud_runtime::body_goals(pud_derivation_id derivation) {
    DEBUG_ASSERT(manifest_.node_descents_.check_descent(derivation.value));
    std::vector<pud_body_goal> goals;
    for (const auto& [id, goal] : manifest_.node_descents_.get(derivation.value).body_goal_expr().body_goal_exprs())
        goals.push_back(pud_body_goal{.id = id, .goal = goal});
    std::sort(goals.begin(), goals.end());
    return goals;
}

size_t pud_runtime::candidate_count(pud_derivation_id derivation, pud_body_goal_id goal) {
    DEBUG_ASSERT(manifest_.open_leaves_.check_leaf_open(derivation.value));
    const std::optional<pud_mhq_head_id> query_head =
        manifest_.node_descents_.get(derivation.value).body_goal_pq().find_query_head(goal);
    DEBUG_ASSERT(query_head.has_value());
    return manifest_.mhq_.get_candidates(query_head.value()).size();
}

pud_whnf<globalizer, expr_pool, expr_pool> pud_runtime::whnf(pud_derivation_id derivation) {
    DEBUG_ASSERT(manifest_.node_descents_.check_descent(derivation.value));
    return pud_whnf<globalizer, expr_pool, expr_pool>{
        manifest_.globalizer_, manifest_.expr_pool_, manifest_.expr_pool_,
        manifest_.node_descents_.get(derivation.value).binding().ubs_state()};
}

pud_frontier_change pud_runtime::collect_change(const std::vector<pud_node_id>& produced) {
    pud_frontier_change change;
    const std::vector<pud_node_id>& refuted_leaves = manifest_.frontier_recorder_.get_refuted_leaves();
    const std::unordered_set<pud_node_id> refuted(refuted_leaves.begin(), refuted_leaves.end());
    for (pud_node_id leaf : manifest_.frontier_recorder_.get_opened_leaves()) {
        if (refuted.contains(leaf))
            continue;
        change.opened.push_back(pud_derivation_id{static_cast<uint32_t>(leaf)});
    }
    for (pud_node_id node : produced) {
        const bool live = manifest_.node_descents_.check_descent(node);
        if (!live || manifest_.open_leaves_.check_leaf_open(node))
            continue;
        change.proven.push_back(pud_derivation_id{static_cast<uint32_t>(node)});
    }
    for (pud_node_id leaf : refuted_leaves)
        change.refuted.push_back(pud_derivation_id{static_cast<uint32_t>(leaf)});
    return change;
}

pud_node_id pud_runtime::find_root(pud_node_id node) {
    pud_node_id ancestor = node;
    while (!manifest_.root_nodes_.check_root(ancestor))
        ancestor = manifest_.node_parents_.get(ancestor);
    return ancestor;
}
