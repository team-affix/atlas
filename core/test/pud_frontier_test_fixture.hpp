#ifndef PUD_FRONTIER_TEST_FIXTURE_HPP
#define PUD_FRONTIER_TEST_FIXTURE_HPP

#include <string>
#include <vector>
#include "functor_fixture.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/rule.hpp"

// Minimal helpers for integration tests that drive register/activate/expand on a real manifest.
struct pud_frontier_test_fixture {
    pud_manifest manifest;
    test_functors functors;
    std::vector<pud_node_id> pending_axiom_roots_;

    const expr* var(uint32_t index) { return manifest.expr_pool_.make_var(index); }

    const expr* atom(const std::string& name) {
        return manifest.expr_pool_.make_functor(functors.id(name), {});
    }

    const expr* functor(const std::string& name, std::vector<const expr*> args) {
        return manifest.expr_pool_.make_functor(functors.id(name), args);
    }

    pud_node_id axiom(const expr* head, std::vector<const expr*> body, uint32_t var_count) {
        const pud_node_id root = manifest.axiom_registrar_.register_axiom(
            rule(head, body, var_count), manifest.database_axiom_group_);
        pending_axiom_roots_.push_back(root);
        return root;
    }

    pud_frontier_change open_axioms() {
        const pud_frontier_change change =
            manifest.axiom_registrar_.activate_axiom_group(manifest.database_axiom_group_, pending_axiom_roots_);
        pending_axiom_roots_.clear();
        return change;
    }

    pud_frontier_change expand(pud_node_id leaf, pud_body_goal_id goal) {
        return manifest.expander_.expand(leaf, goal);
    }

    const std::vector<pud_node_id>& children_of(pud_node_id node) const {
        return manifest.children_.get(node);
    }

    bool refuted(pud_node_id node) const { return manifest.refuted_node_checker_.check_refuted(node); }

    void add_colors() {
        axiom(functor("color", {atom("red")}), {}, 0);
        axiom(functor("color", {atom("blue")}), {}, 0);
        axiom(functor("warm", {atom("red")}), {}, 0);
    }
};

#endif
