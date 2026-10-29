#include "infrastructure/pud_lodestar_manifest.hpp"
#include "value_objects/framed_functor.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/ubs_class.hpp"

pud_lodestar_manifest::pud_lodestar_manifest(
    expr_pool& expr_pool,
    pud_global_node_id focus_axiom,
    size_t max_unfolds,
    size_t non_conjecture_unfolds_per_conjecture_unfold)
    : globalizer_()
    , cycle_detector_factory_(globalizer_, expr_pool)
    , unifier_factory_(globalizer_, expr_pool)
    , exporter_factory_(globalizer_, expr_pool, expr_pool)
    , bind_map_factory_()
    , modules_()
    , node_goals_()
    , node_axioms_()
    , live_child_counts_()
    , refutation_propagator_(modules_, live_child_counts_, modules_)
    , unit_queries_()
    , replayer_(modules_, modules_, expr_pool, bind_map_factory_, cycle_detector_factory_, unifier_factory_)
    , mhws_(replayer_, modules_, modules_, modules_)
    , witness_log_(mhws_, modules_)
    , mhcs_(replayer_, witness_log_, witness_log_, witness_log_, witness_log_, witness_log_, modules_, modules_, modules_)
    , candidate_watch_(mhcs_, witness_log_)
    , mhq_(replayer_, modules_, candidate_watch_, candidate_watch_, candidate_watch_, candidate_watch_, unit_queries_)
    , active_states_()
    , query_holders_()
    , binder_(bind_map_factory_, modules_)
    , structure_watcher_(bind_map_factory_, modules_, expr_pool)
    , root_activator_(
          modules_, initial_goal_opener_, node_goals_, node_axioms_, active_states_, structure_watcher_, mhq_,
          query_holders_)
    , child_activator_(
          active_states_, active_states_, modules_, node_goals_, node_axioms_, goal_resolver_, node_goals_,
          node_goals_, node_axioms_, live_child_counts_, binder_, structure_watcher_, mhq_, mhq_, query_holders_,
          mhq_)
    , node_deactivator_(active_states_, active_states_, query_holders_, mhq_, mhq_, query_holders_)
    , leaf_refuter_(modules_, refutation_propagator_, node_deactivator_, modules_, modules_)
    , acceptor_(
          node_goals_, modules_, modules_, expr_pool, expr_pool, bind_map_factory_, cycle_detector_factory_,
          unifier_factory_, exporter_factory_)
    , resolver_(active_states_, node_goals_, mhq_, mhcs_, acceptor_)
    , unfolder_(resolver_, modules_, child_activator_, leaf_refuter_, node_deactivator_, modules_)
    , module_activator_(
          modules_, modules_, modules_, root_activator_, child_activator_, leaf_refuter_, node_deactivator_)
    , exprs_(expr_pool)
    , focus_axiom_(focus_axiom)
    , max_unfolds_(max_unfolds)
    , unfolds_(0)
    , entry_unfolds_(0)
    , pending_conflicts_(0)
    , frontier_()
    , driver_(*this, frontier_, focus_axiom, non_conjecture_unfolds_per_conjecture_unfold)
    , reported_()
    , solution_(nullptr)
    , solution_depth_(0)
    , stopped_(false) {}

void pud_lodestar_manifest::register_module(pud_module& module) {
    modules_.register_module(module);
    const pud_frontier_change change = module_activator_.activate_module(module.get_module_id());
    frontier_.apply_register(change);
    if (!change.refuted.empty())
        pending_conflicts_ += change.refuted.size();
}

std::optional<pud_global_node_id> pud_lodestar_manifest::unreported_answer() const {
    std::optional<pud_global_node_id> smallest;
    for (const pud_global_node_id leaf : frontier_.live()) {
        if (node_axioms_.get(leaf) != focus_axiom_)
            continue;
        if (modules_.check_refuted(leaf))
            continue;
        if (reported_.contains(leaf))
            continue;
        if (!node_goals_.get(leaf).goals.empty())
            continue;
        if (smallest.has_value() && leaf >= smallest.value())
            continue;
        smallest = leaf;
    }
    return smallest;
}

const expr* pud_lodestar_manifest::normalize_with_bindings(
    const ubs_bind_map& bind_map, const expr* skeleton) const {
    if (const expr::var* variable = std::get_if<expr::var>(&skeleton->content)) {
        const ubs_class variable_class = bind_map.find(variable->index);
        if (!variable_class.value.has_value())
            return exprs_.make_var(variable_class.rep);
        const expr::functor& functor = *variable_class.value->functor;
        const expr* parent = exprs_.make_functor(functor.id, functor.args);
        return normalize_with_bindings(bind_map, parent);
    }
    const expr::functor& term = std::get<expr::functor>(skeleton->content);
    std::vector<const expr*> args;
    for (const expr* arg : term.args)
        args.push_back(normalize_with_bindings(bind_map, arg));
    return exprs_.make_functor(term.id, args);
}

const expr* pud_lodestar_manifest::normalized_head(pud_global_node_id leaf) const {
    const rule& axiom = modules_.get_axiom(node_axioms_.get(leaf));
    const ubs_bind_map bindings{active_states_.get(leaf).bindings};
    return normalize_with_bindings(bindings, axiom.head);
}

size_t pud_lodestar_manifest::depth_of(pud_global_node_id leaf) const {
    size_t depth = 0;
    std::optional<pud_global_node_id> cursor = leaf;
    while (cursor.has_value()) {
        cursor = modules_.get_parent(cursor.value());
        if (!cursor.has_value())
            break;
        ++depth;
    }
    return depth;
}

bool pud_lodestar_manifest::next() {
    solution_ = nullptr;
    solution_depth_ = 0;
    if (stopped_)
        return false;
    if (pending_conflicts_ > 0) {
        --pending_conflicts_;
        return true;
    }
    for (;;) {
        const std::optional<pud_global_node_id> answer = unreported_answer();
        if (answer.has_value()) {
            solution_ = normalized_head(answer.value());
            solution_depth_ = depth_of(answer.value());
            reported_.insert(answer.value());
            return true;
        }
        if (driver_.refute_blocked())
            return true;
        if (entry_unfolds_ >= max_unfolds_) {
            stopped_ = true;
            return reported_.empty();
        }
        if (!driver_.unfold_one()) {
            stopped_ = true;
            return false;
        }
        ++unfolds_;
        if (driver_.last_unfold_on_focus())
            ++entry_unfolds_;
    }
}

const expr* pud_lodestar_manifest::solution() const {
    return solution_;
}

size_t pud_lodestar_manifest::solution_depth() const {
    return solution_depth_;
}

