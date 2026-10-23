#ifndef PUD_FRONTIER_TEST_FIXTURE_HPP
#define PUD_FRONTIER_TEST_FIXTURE_HPP

#include <string>
#include <vector>
#include "functor_fixture.hpp"
#include "infrastructure/pud_manifest.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/rule.hpp"

// Minimal helpers for integration tests that make, activate, and expand nodes on a real manifest.
// Axioms are made as they are declared and activated together by activate_axioms.
struct pud_frontier_test_fixture {
    pud_manifest manifest;
    test_functors functors;
    std::vector<pud_node_id> unactivated_roots;

    const expr* var(uint32_t index) { return manifest.expr_pool_.make_var(index); }

    const expr* atom(const std::string& name) {
        return manifest.expr_pool_.make_functor(functors.id(name), {});
    }

    const expr* functor(const std::string& name, std::vector<const expr*> args) {
        return manifest.expr_pool_.make_functor(functors.id(name), args);
    }

    pud_node_id axiom(const expr* head, std::vector<const expr*> body, uint32_t var_count) {
        const pud_node_id root = manifest.node_maker_.make_root(rule(head, body, var_count), manifest.database_axiom_group_);
        unactivated_roots.push_back(root);
        return root;
    }

    pud_frontier_change activate_axioms() {
        pud_frontier_change change{.offspring = {}, .refuted = {}};
        for (pud_node_id root : unactivated_roots) {
            if (!manifest.root_activator_.activate_root(root))
                continue;
            const std::vector<pud_node_id> refuted = manifest.leaf_refuter_.refute(root);
            change.refuted.insert(change.refuted.end(), refuted.begin(), refuted.end());
        }
        for (pud_node_id root : unactivated_roots) {
            if (manifest.refuted_leaves_.check_refuted(root))
                continue;
            change.offspring.push_back(root);
        }
        unactivated_roots.clear();
        return change;
    }

    pud_frontier_change expand(pud_node_id leaf, pud_body_goal_id goal) {
        return manifest.expander_.expand(leaf, goal);
    }

    const std::vector<pud_node_id>& children_of(pud_node_id node) const {
        return manifest.data_.get_children(node);
    }

    bool refuted(pud_node_id node) const { return manifest.refuted_node_checker_.check_refuted(node); }

    void add_colors() {
        axiom(functor("color", {atom("red")}), {}, 0);
        axiom(functor("color", {atom("blue")}), {}, 0);
        axiom(functor("warm", {atom("red")}), {}, 0);
    }
};

#endif
