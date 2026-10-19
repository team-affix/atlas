#include "infrastructure/pud_runtime.hpp"

#include <algorithm>
#include <set>
#include <unordered_set>
#include <utility>
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

pud_runtime::pud_runtime(db& database)
    : database_(database)
    , database_opened_(false)
    , manifest_() {}

pud_frontier_change pud_runtime::open_database() {
    DEBUG_ASSERT(!database_opened_);
    database_opened_ = true;
    clear_recorders();
    const pud_node_id first_new_node = manifest_.node_id_sequencer_.peek();

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
    return collect_change(first_new_node, roots);
}

pud_frontier_change pud_runtime::add_conjecture(
    const expr* head, const initial_goal_exprs& goals, uint32_t var_count) {
    DEBUG_ASSERT(database_opened_);
    clear_recorders();
    const pud_node_id first_new_node = manifest_.node_id_sequencer_.peek();

    std::vector<const expr*> goal_exprs;
    for (size_t index = 0; index < goals.count(); ++index)
        goal_exprs.push_back(goals.get(index));
    const pud_node_id root = manifest_.conjecture_initializer_.initialize_conjecture(head, goal_exprs, var_count);
    manifest_.base_descent_runner_.run(root);
    return collect_change(first_new_node, {root});
}

pud_frontier_change pud_runtime::unfold(pud_derivation_id derivation, pud_body_goal_id goal) {
    const pud_node_id leaf = derivation.value;
    DEBUG_ASSERT(manifest_.open_leaves_.check_leaf_open(leaf));
    DEBUG_ASSERT(manifest_.node_descents_.get(leaf).body_goal_pq().find_query_head(goal).has_value());
    clear_recorders();
    const pud_node_id first_new_node = manifest_.node_id_sequencer_.peek();

    manifest_.unfolder_.unfold(leaf, goal);
    return collect_change(first_new_node, manifest_.children_.get(leaf));
}

coroutine<pud_open_goal, void> pud_runtime::iterate_body_goals(pud_derivation_id derivation) {
    const pud_node_id leaf = derivation.value;
    DEBUG_ASSERT(manifest_.open_leaves_.check_leaf_open(leaf));
    std::vector<std::pair<pud_body_goal_id, pud_mhq_head_id>> query_heads =
        manifest_.node_descents_.get(leaf).body_goal_pq().get_query_heads();
    std::sort(query_heads.begin(), query_heads.end());
    for (const auto& [goal, head] : query_heads)
        co_yield pud_open_goal{.goal = goal, .candidate_count = manifest_.mhq_.get_candidates(head).size()};
}

const expr* pud_runtime::head_expr(pud_derivation_id derivation) {
    const pud_node_id leaf = derivation.value;
    DEBUG_ASSERT(manifest_.node_descents_.check_descent(leaf));
    return normalize_at_leaf(leaf, manifest_.node_heads_.get(find_root(leaf)));
}

const expr* pud_runtime::body_goal_expr(pud_derivation_id derivation, pud_body_goal_id goal) {
    const pud_node_id leaf = derivation.value;
    DEBUG_ASSERT(manifest_.open_leaves_.check_leaf_open(leaf));
    const auto& descent = manifest_.node_descents_.get(leaf);
    DEBUG_ASSERT(descent.body_goal_pq().find_query_head(goal).has_value());
    return normalize_at_leaf(leaf, descent.body_goal_expr().body_goal_exprs().at(goal));
}

void pud_runtime::clear_recorders() {
    manifest_.frontier_recorder_.clear();
    while (!manifest_.unit_queries_.empty())
        manifest_.unit_queries_.pop();
}

pud_frontier_change pud_runtime::collect_change(
    pud_node_id first_new_node, const std::vector<pud_node_id>& produced) {
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

    std::set<pud_mhq_head_id> unit_heads;
    while (!manifest_.unit_queries_.empty()) {
        unit_heads.insert(manifest_.unit_queries_.front());
        manifest_.unit_queries_.pop();
    }
    for (pud_mhq_head_id head : unit_heads) {
        if (!manifest_.mhq_.check_head(head))
            continue;
        const bool unit = manifest_.mhq_.get_candidates(head).size() == 1;
        if (!unit)
            continue;
        for (const auto& [leaf, goal] : manifest_.query_holders_.get(head)) {
            const bool new_leaf = leaf >= first_new_node;
            if (new_leaf)
                continue;
            change.made_unit.push_back(pud_unit_goal{
                .derivation = pud_derivation_id{static_cast<uint32_t>(leaf)},
                .goal = goal});
        }
    }
    std::sort(change.made_unit.begin(), change.made_unit.end());
    return change;
}

pud_node_id pud_runtime::find_root(pud_node_id node) {
    pud_node_id ancestor = node;
    while (!manifest_.root_nodes_.check_root(ancestor))
        ancestor = manifest_.node_parents_.get(ancestor);
    return ancestor;
}

const expr* pud_runtime::normalize_at_leaf(pud_node_id leaf, const expr* skeleton) {
    pud_transient_ubs_state transient_ubs_state{manifest_.node_descents_.get(leaf).binding().ubs_state()};
    auto bind_map = transient_ubs_state.make_bind_map(manifest_.expr_pool_, manifest_.expr_pool_);
    auto normalizer = manifest_.normalizer_factory_.make(bind_map);
    return normalizer.normalize(framed_expr{skeleton, 0});
}
