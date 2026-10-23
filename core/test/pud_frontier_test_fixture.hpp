#ifndef PUD_FRONTIER_TEST_FIXTURE_HPP
#define PUD_FRONTIER_TEST_FIXTURE_HPP

#include <deque>
#include <stdexcept>
#include <string>
#include <vector>
#include "functor_fixture.hpp"
#include "pud_test_node.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_controller.hpp"
#include "infrastructure/pud_data.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/rule.hpp"

// Builds modules, registers them with a real controller, and expands their leaves. Axioms are
// declared into the module being built and registered with it by register_module; the next module
// is then built. The registered modules' data is the caller's, so tests read it directly.
struct pud_frontier_test_fixture {
    expr_pool exprs;
    test_functors functors;
    std::deque<pud_data> registered;
    pud_controller<pud_data, expr_pool> controller{exprs};
    pud_module_id building = 1;
    std::vector<pud_module_id> building_dependencies;
    std::vector<rule> building_axioms;

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* atom(const std::string& name) {
        return exprs.make_functor(functors.id(name), {});
    }

    const expr* functor(const std::string& name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), args);
    }

    pud_global_node_id axiom(const expr* head, std::vector<const expr*> body, uint32_t var_count) {
        building_axioms.push_back(rule(head, std::move(body), var_count));
        return pud_global_node_id{.module = building, .node = building_axioms.size() - 1};
    }

    void depend_on(pud_module_id dependency) { building_dependencies.push_back(dependency); }

    pud_frontier_change register_module() {
        pud_data& data = registered.emplace_back(building, building_dependencies, building_axioms);
        building_dependencies.clear();
        building_axioms.clear();
        ++building;
        return controller.register_module(data);
    }

    pud_frontier_change expand(pud_global_node_id leaf, pud_body_goal_id goal) {
        return controller.expand(leaf, goal);
    }

    const pud_data& data_of(pud_module_id module) const {
        for (const pud_data& data : registered) {
            if (data.get_module_id() == module)
                return data;
        }
        throw std::out_of_range("module not registered");
    }

    std::vector<pud_global_node_id> children_of(pud_global_node_id node) const {
        std::vector<pud_global_node_id> children;
        for (pud_node_id child : data_of(node.module).get_children(node.node))
            children.push_back(pud_global_node_id{.module = node.module, .node = child});
        return children;
    }

    bool refuted(pud_global_node_id node) const { return data_of(node.module).check_refuted(node.node); }

    std::vector<pud_global_node_id> add_colors() {
        return {
            axiom(functor("color", {atom("red")}), {}, 0),
            axiom(functor("color", {atom("blue")}), {}, 0),
            axiom(functor("warm", {atom("red")}), {}, 0),
        };
    }
};

#endif
