#ifndef PUD_SYSTEM_FIXTURE_HPP
#define PUD_SYSTEM_FIXTURE_HPP

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include <gtest/gtest.h>
#include <immer/map_transient.hpp>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/expr_printer.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/hierarchical_bind_map.hpp"
#include "infrastructure/named_functor_printer.hpp"
#include "infrastructure/named_var_printer.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/pud_axiom_initializer.hpp"
#include "infrastructure/pud_call_sites.hpp"
#include "infrastructure/pud_children.hpp"
#include "infrastructure/pud_descender.hpp"
#include "infrastructure/pud_leaves.hpp"
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "infrastructure/pud_node_added_specializations.hpp"
#include "infrastructure/pud_node_added_var_count.hpp"
#include "infrastructure/pud_node_heads.hpp"
#include "infrastructure/pud_node_id_sequencer.hpp"
#include "infrastructure/pud_refuted_nodes.hpp"
#include "infrastructure/pud_roots.hpp"
#include "infrastructure/pud_specializer.hpp"
#include "infrastructure/unifier.hpp"
#include "infrastructure/var_names.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_descent.hpp"
#include "value_objects/rule.hpp"

struct PudSystemFixture : public ::testing::Test {
    using bind_map_t    = hierarchical_bind_map<globalizer, immer::map<uint32_t, framed_expr>::transient_type>;
    using unifier_t     = unifier<globalizer, bind_map_t>;
    using specializer_t = pud_specializer<expr_pool, unifier_t>;
    using normalizer_t  = normalizer<globalizer, expr_pool, expr_pool, bind_map_t>;
    using descender_t   = pud_descender<
        bind_map_t,
        unifier_t,
        specializer_t,
        normalizer_t,
        pud_node_id_sequencer,
        expr_pool,
        globalizer,
        pud_refuted_nodes,
        pud_call_sites,
        pud_node_added_specializations,
        pud_node_added_body_goals,
        pud_node_added_var_count,
        pud_node_heads,
        pud_node_added_specializations,
        pud_node_added_body_goals,
        pud_node_added_var_count>;
    using initializer_t = pud_axiom_initializer<
        pud_node_id_sequencer,
        pud_node_heads,
        pud_node_added_body_goals,
        pud_node_added_var_count,
        pud_roots>;

    test_functors                  functors;
    expr_pool                      exprs;
    globalizer                     globalize;
    var_names                      var_names_;
    pud_node_id_sequencer          sequencer_;
    pud_node_heads                 node_heads_;
    pud_node_added_specializations node_specs_;
    pud_node_added_body_goals      node_goals_;
    pud_node_added_var_count       node_var_counts_;
    pud_roots                      roots_;
    pud_refuted_nodes              refuted_;
    pud_call_sites                 call_sites_;
    pud_children                   children_;
    pud_leaves                     leaves_;
    std::unordered_set<pud_node_id> call_site_registered_;
    descender_t                    descender_;
    initializer_t                  axiom_initializer_;

    PudSystemFixture()
        : descender_(sequencer_, exprs, globalize, refuted_, call_sites_,
                     node_specs_, node_goals_, node_var_counts_, node_heads_,
                     node_specs_, node_goals_, node_var_counts_)
        , axiom_initializer_(sequencer_, node_heads_, node_goals_, node_var_counts_, roots_) {}

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* fn(const char* name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), std::move(args));
    }

    const expr* list(std::vector<const expr*> elems, const expr* tail = nullptr) {
        const expr* current = tail;
        if (!current)
            current = exprs.make_functor(k_nil_functor_id, {});
        for (auto it = elems.rbegin(); it != elems.rend(); ++it)
            current = exprs.make_functor(k_cons_functor_id, {*it, current});
        return current;
    }

    const expr* nat(uint32_t n) {
        const expr* current = fn("z", {});
        for (uint32_t step = 0; step < n; ++step)
            current = fn("s", {current});
        return current;
    }

    pud_node_id axiom(const expr* head, std::vector<const expr*> body, uint32_t var_count) {
        return axiom_initializer_.initialize_axiom(rule(head, std::move(body), var_count));
    }

    std::string show(const expr* expression) {
        std::ostringstream os;
        named_var_printer<var_names> print_var{var_names_};
        named_functor_printer<functor_names> print_functor{functors.names};
        expr_printer<named_var_printer<var_names>, named_functor_printer<functor_names>> printer{
            os, print_var, print_functor};
        printer.print(expression);
        return os.str();
    }

    std::vector<std::string> specs_of(pud_node_id node) {
        std::vector<std::string> lines;
        for (const pud_specialization& spec : node_specs_.get(node)) {
            std::ostringstream os;
            os << "?" << spec.var_idx << "=" << show(spec.value);
            lines.push_back(os.str());
        }
        std::sort(lines.begin(), lines.end());
        return lines;
    }

    std::vector<std::string> goals_of(pud_node_id node) {
        std::vector<std::string> lines;
        const std::vector<const expr*>& goals = node_goals_.get(node);
        for (size_t goal_index = 0; goal_index < goals.size(); ++goal_index) {
            std::ostringstream os;
            os << goal_index << ": " << show(goals[goal_index]);
            lines.push_back(os.str());
        }
        return lines;
    }

    bool expr_eq(const expr* left, const expr* right) {
        return show(left) == show(right);
    }

    const expr* value_of_global(const pud_descent& descent, uint32_t global_key) {
        auto bindings_transient = descent.bindings.transient();
        bind_map_t bind_map{globalize, bindings_transient};
        normalizer_t normalizer{globalize, exprs, exprs, bind_map};
        const uint32_t cutoff = descent.frame_offset + descent.lvc;
        std::unordered_map<uint32_t, uint32_t> translation;
        framed_expr raw{exprs.make_var(global_key), 0};
        framed_expr reduced = bind_map.whnf(raw);
        return normalizer.normalize(reduced, cutoff, translation);
    }

    const expr* value_of(const pud_descent& descent, uint32_t local_var_index) {
        const uint32_t global_key = globalize.globalize(descent.frame_offset, local_var_index);
        return value_of_global(descent, global_key);
    }

    void ensure_call_site(pud_node_id parent, body_goal_id call_site) {
        const bool already_registered = call_site_registered_.contains(parent);
        if (already_registered)
            return;
        call_sites_.store(parent, call_site);
        call_site_registered_.insert(parent);
    }

    void set_children(
        pud_node_id parent,
        std::vector<pud_node_id> kids,
        body_goal_id call_site = std::numeric_limits<size_t>::max()) {
        children_.store(parent, std::move(kids));
        ensure_call_site(parent, call_site);
    }

    void attach_child(pud_node_id parent, pud_node_id child, body_goal_id call_site) {
        if (children_.get(parent).empty())
            children_.store(parent, {child});
        ensure_call_site(parent, call_site);
        if (leaves_.check_leaf(parent))
            leaves_.unset_leaf(parent);
        leaves_.set_leaf(child);
    }

    std::optional<pud_node_id> resolve(
        const pud_descent& caller,
        body_goal_id goal_id,
        pud_node_id axiom_id) {
        const expr* const* goal = caller.pending_body_goals.find(goal_id);
        if (!goal)
            return std::nullopt;
        std::optional<pud_descent> opened = descender_.open_query(caller, *goal, axiom_id);
        if (!opened)
            return std::nullopt;
        const pud_node_id child = descender_.close_query(*opened);
        attach_child(caller.node, child, goal_id);
        return child;
    }

    std::optional<pud_descent> walk(pud_node_id root_id, const std::vector<pud_node_id>& path) {
        pud_descent current = descender_.descent_root(root_id);
        for (pud_node_id step : path) {
            std::optional<pud_descent> next = descender_.descend(current, step);
            if (!next)
                return std::nullopt;
            current = *next;
        }
        return current;
    }

    std::optional<pud_descent> open_at_root(pud_node_id root_id, const expr* query) {
        pud_descent root_descent = descender_.descent_root(root_id);
        return descender_.open_query(root_descent, query, root_id);
    }

    pud_node_id close_opened(const pud_descent& opened) {
        return descender_.close_query(opened);
    }

    pud_node_id store_closed_node(
        std::vector<pud_specialization> specs,
        std::vector<const expr*> goals,
        uint32_t var_count) {
        const pud_node_id id = sequencer_.next();
        node_specs_.store(id, std::move(specs));
        node_goals_.store(id, std::move(goals));
        node_var_counts_.store(id, var_count);
        return id;
    }

    void link_descend_only(
        pud_node_id parent,
        pud_node_id child,
        body_goal_id call_site = std::numeric_limits<size_t>::max()) {
        if (children_.get(parent).empty())
            children_.store(parent, {child});
        ensure_call_site(parent, call_site);
    }
};

#endif
