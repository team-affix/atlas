#include <deque>
#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "reference_pud_normalize_with_bindings.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_runtime.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/rule.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

struct PudRuntimeRuleInformationIntegrationTest : public ::testing::Test {
    expr_pool exprs;
    test_functors functors;
    std::deque<pud_module> modules;
    pud_runtime<pud_module, expr_pool> runtime{exprs};

    const expr* var(uint32_t index) { return exprs.make_var(index); }

    const expr* atom(const std::string& name) {
        return exprs.make_functor(functors.id(name), {});
    }

    const expr* functor(const std::string& name, std::vector<const expr*> args) {
        return exprs.make_functor(functors.id(name), args);
    }

    void register_nat_and_query_modules(pud_global_node_id& nat_step_axiom, pud_global_node_id& query_axiom) {
        pud_module& nat = modules.emplace_back(
            1,
            std::vector<pud_module_id>{},
            std::vector<rule>{
                rule(functor("nat", {atom("z")}), {}, 0),
                rule(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1),
            });
        install_shared_functor_names(nat, this->functors);
        runtime.register_module(nat);
        nat_step_axiom = pud_global_node_id{.module = 1, .node = 1};
        pud_module& query_mod = modules.emplace_back(
            2,
            std::vector<pud_module_id>{nat_step_axiom.module},
            std::vector<rule>{rule(functor("q", {var(0)}), {functor("nat", {var(0)})}, 1)});
        install_shared_functor_names(query_mod, this->functors);
        runtime.register_module(query_mod);
        query_axiom = pud_global_node_id{.module = 2, .node = 0};
    }

    bool normalized_head_is_query_with_successor_depth(const pud_unnormalized_rule_information& info, size_t depth) {
        const expr* normalized = reference_pud_normalize_with_bindings(info.bindings, exprs, info.head);
        const expr::functor* query = std::get_if<expr::functor>(&normalized->content);
        if (query == nullptr || query->id != functors.id("q") || query->args.size() != 1)
            return false;
        const expr* argument = query->args.at(0);
        for (size_t level = 0; level < depth; ++level) {
            const expr::functor* successor = std::get_if<expr::functor>(&argument->content);
            if (successor == nullptr || successor->id != functors.id("s") || successor->args.size() != 1)
                return false;
            argument = successor->args.at(0);
        }
        const expr::functor* zero = std::get_if<expr::functor>(&argument->content);
        const bool ground_zero =
            zero != nullptr && zero->id == functors.id("z") && zero->args.empty();
        return std::get_if<expr::var>(&argument->content) != nullptr || ground_zero;
    }

    pud_body_goal_id sole_open_goal(const pud_unnormalized_rule_information& info) {
        pud_body_goal_id goal = 0;
        bool found = false;
        for (const auto& entry : info.body) {
            EXPECT_FALSE(found);
            found = true;
            goal = entry.first;
        }
        EXPECT_TRUE(found);
        return goal;
    }

    pud_global_node_id find_query_child_with_successor_depth(
        const std::vector<pud_global_node_id>& offspring, size_t depth) {
        for (const pud_global_node_id node : offspring) {
            const pud_unnormalized_rule_information info = runtime.get_rule_information(node);
            if (normalized_head_is_query_with_successor_depth(info, depth))
                return node;
        }
        ADD_FAILURE() << "no live child has the expected normalized query shape";
        return offspring.at(0);
    }

};

TEST_F(PudRuntimeRuleInformationIntegrationTest, QueryAxiomReportsItsHeadBodyAndNormalizedHead) {
    pud_global_node_id nat_step{};
    pud_global_node_id query{};
    register_nat_and_query_modules(nat_step, query);

    const pud_unnormalized_rule_information info = runtime.get_rule_information(query);
    const immer::map<pud_body_goal_id, const expr*> expected_body =
        immer::map<pud_body_goal_id, const expr*>{}.set(0, functor("nat", {var(0)}));
    EXPECT_EQ(info.head, functor("q", {var(0)}));
    EXPECT_EQ(info.body, expected_body);
    EXPECT_EQ(reference_pud_normalize_with_bindings(info.bindings, exprs, info.head), functor("q", {var(0)}));
}

TEST_F(PudRuntimeRuleInformationIntegrationTest, OneStepChildKeepsQueryHeadWithBindingsFromUnfold) {
    pud_global_node_id nat_step{};
    pud_global_node_id query{};
    register_nat_and_query_modules(nat_step, query);

    const pud_frontier_change first = runtime.unfold(query, 0);
    const pud_global_node_id child = find_query_child_with_successor_depth(first.offspring, 1);

    const pud_unnormalized_rule_information info = runtime.get_rule_information(child);
    EXPECT_EQ(info.head, functor("q", {var(0)}));
    EXPECT_FALSE(info.body.empty());
    EXPECT_TRUE(normalized_head_is_query_with_successor_depth(info, 1));
}

TEST_F(PudRuntimeRuleInformationIntegrationTest, DeepQueryChildNormalizesUnderBindings) {
    pud_global_node_id nat_step{};
    pud_global_node_id query{};
    register_nat_and_query_modules(nat_step, query);

    pud_global_node_id query_child = find_query_child_with_successor_depth(runtime.unfold(query, 0).offspring, 1);
    query_child = find_query_child_with_successor_depth(
        runtime.unfold(query_child, sole_open_goal(runtime.get_rule_information(query_child))).offspring, 2);

    const pud_unnormalized_rule_information info = runtime.get_rule_information(query_child);
    EXPECT_EQ(info.head, functor("q", {var(0)}));
    EXPECT_TRUE(normalized_head_is_query_with_successor_depth(info, 2));
}

TEST_F(PudRuntimeRuleInformationIntegrationTest, TwoOpenBodyGoalsAppearInBodyMap) {
    pud_global_node_id nat_step{};
    pud_global_node_id query{};
    pud_module& nat = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{
            rule(functor("nat", {atom("z")}), {}, 0),
            rule(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1),
            rule(functor("color", {atom("red")}), {}, 0),
        });
    install_shared_functor_names(nat, this->functors);
    runtime.register_module(nat);
    nat_step = pud_global_node_id{.module = 1, .node = 1};
    pud_module& query_mod = modules.emplace_back(
        2,
        std::vector<pud_module_id>{nat_step.module},
        std::vector<rule>{
            rule(functor("q", {var(0)}), {functor("nat", {var(0)}), functor("color", {var(0)})}, 1)});
    install_shared_functor_names(query_mod, this->functors);
    runtime.register_module(query_mod);
    query = pud_global_node_id{.module = 2, .node = 0};

    const pud_unnormalized_rule_information info = runtime.get_rule_information(query);
    EXPECT_EQ(*info.body.find(0), functor("nat", {var(0)}));
    EXPECT_EQ(*info.body.find(1), functor("color", {var(0)}));
}

TEST_F(PudRuntimeRuleInformationIntegrationTest, RepeatedVariablesEquateInBindings) {
    pud_module& base = modules.emplace_back(
        1,
        std::vector<pud_module_id>{},
        std::vector<rule>{rule(functor("p", {var(0), var(0)}), {}, 1)});
    install_shared_functor_names(base, this->functors);
    runtime.register_module(base);
    const pud_global_node_id query = pud_global_node_id{.module = 2, .node = 0};
    pud_module& query_mod = modules.emplace_back(
        2,
        std::vector<pud_module_id>{base.get_module_id()},
        std::vector<rule>{rule(functor("q", {var(0), var(1)}), {functor("p", {var(0), var(1)})}, 2)});
    install_shared_functor_names(query_mod, this->functors);
    runtime.register_module(query_mod);

    const pud_global_node_id child = runtime.unfold(query, 0).offspring.front();
    const pud_unnormalized_rule_information info = runtime.get_rule_information(child);
    EXPECT_EQ(info.head, functor("q", {var(0), var(1)}));
    const ubs_class first = info.bindings.find(0);
    const ubs_class second = info.bindings.find(1);
    EXPECT_EQ(first.root, second.root);
    EXPECT_EQ(
        reference_pud_normalize_with_bindings(info.bindings, exprs, var(0)),
        reference_pud_normalize_with_bindings(info.bindings, exprs, var(1)));
}

TEST_F(PudRuntimeRuleInformationIntegrationTest, ModuleTwoNodeUsesModuleTwoAxiomHead) {
    pud_global_node_id nat_step{};
    pud_global_node_id query{};
    register_nat_and_query_modules(nat_step, query);

    const pud_unnormalized_rule_information query_info = runtime.get_rule_information(query);
    EXPECT_EQ(query_info.head, functor("q", {var(0)}));

    const pud_global_node_id nat_child = runtime.unfold(nat_step, 0).offspring.front();
    const pud_unnormalized_rule_information nat_info = runtime.get_rule_information(nat_child);
    EXPECT_EQ(nat_info.head, functor("nat", {functor("s", {var(0)})}));
    EXPECT_NE(nat_info.head, query_info.head);
}

TEST_F(PudRuntimeRuleInformationIntegrationTest, DeepNatChainNormalizesToNestedSuccessors) {
    pud_global_node_id nat_step{};
    pud_global_node_id query{};
    register_nat_and_query_modules(nat_step, query);

    pud_global_node_id leaf = query;
    for (size_t depth = 1; depth <= 12; ++depth) {
        const pud_unnormalized_rule_information before = runtime.get_rule_information(leaf);
        pud_body_goal_id open_goal = 0;
        bool found_open_goal = false;
        for (const auto& entry : before.body) {
            EXPECT_FALSE(found_open_goal);
            found_open_goal = true;
            open_goal = entry.first;
        }
        EXPECT_TRUE(found_open_goal);

        const pud_frontier_change change = runtime.unfold(leaf, open_goal);
        pud_global_node_id successor = {};
        for (const pud_global_node_id node : change.offspring) {
            const pud_unnormalized_rule_information child_info = runtime.get_rule_information(node);
            if (!child_info.body.empty())
                successor = node;
        }
        EXPECT_FALSE(successor == pud_global_node_id{});
        leaf = successor;

        const pud_unnormalized_rule_information info = runtime.get_rule_information(leaf);
        EXPECT_EQ(info.body.size(), static_cast<size_t>(1));

        const expr* body_goal_expr = nullptr;
        bool found_body_goal = false;
        for (const auto& entry : info.body) {
            EXPECT_FALSE(found_body_goal);
            found_body_goal = true;
            body_goal_expr = entry.second;
        }
        EXPECT_TRUE(found_body_goal);

        const expr::functor& body_nat = std::get<expr::functor>(body_goal_expr->content);
        EXPECT_EQ(body_nat.id, functors.id("nat"));
        const expr* v_from_body =
            reference_pud_normalize_with_bindings(info.bindings, exprs, body_nat.args.at(0));
        EXPECT_NE(std::get_if<expr::var>(&v_from_body->content), nullptr);

        const expr* normalized_head = reference_pud_normalize_with_bindings(info.bindings, exprs, info.head);
        const expr::functor& query_head = std::get<expr::functor>(normalized_head->content);
        EXPECT_EQ(query_head.id, functors.id("q"));
        const expr* peeled = query_head.args.at(0);
        for (size_t level = 0; level < depth; ++level) {
            const expr::functor& successor_functor = std::get<expr::functor>(peeled->content);
            EXPECT_EQ(successor_functor.id, functors.id("s"));
            peeled = successor_functor.args.at(0);
        }
        EXPECT_EQ(peeled, v_from_body);

        const expr* expected_inner = v_from_body;
        for (size_t level = 0; level < depth; ++level)
            expected_inner = functor("s", {expected_inner});
        EXPECT_EQ(normalized_head, functor("q", {expected_inner}));
        EXPECT_EQ(
            reference_pud_normalize_with_bindings(info.bindings, exprs, body_goal_expr),
            functor("nat", {v_from_body}));

        if (depth == 12) {
            pud_body_goal_id final_open_goal = 0;
            bool found_final_goal = false;
            for (const auto& entry : info.body) {
                EXPECT_FALSE(found_final_goal);
                found_final_goal = true;
                final_open_goal = entry.first;
            }
            EXPECT_TRUE(found_final_goal);

            const pud_frontier_change final_change = runtime.unfold(leaf, final_open_goal);
            pud_global_node_id z_solution = {};
            for (const pud_global_node_id node : final_change.offspring) {
                const pud_unnormalized_rule_information child_info = runtime.get_rule_information(node);
                if (child_info.body.empty())
                    z_solution = node;
            }
            EXPECT_FALSE(z_solution == pud_global_node_id{});

            const pud_unnormalized_rule_information solved = runtime.get_rule_information(z_solution);
            EXPECT_TRUE(solved.body.empty());
            const expr* ground_inner = atom("z");
            for (size_t level = 0; level < depth; ++level)
                ground_inner = functor("s", {ground_inner});
            EXPECT_EQ(
                reference_pud_normalize_with_bindings(solved.bindings, exprs, solved.head),
                functor("q", {ground_inner}));
        }
    }
}

TEST_F(PudRuntimeRuleInformationIntegrationTest, ManyUnfoldsKeepEveryLiveNodeReadable) {
    pud_global_node_id nat_step{};
    pud_global_node_id query{};
    register_nat_and_query_modules(nat_step, query);

    std::set<pud_global_node_id> live;
    live.insert(query);

    pud_global_node_id leaf = query;
    for (size_t step = 0; step < 200; ++step) {
        const pud_unnormalized_rule_information leaf_info = runtime.get_rule_information(leaf);
        pud_body_goal_id open_goal = 0;
        bool found_open_goal = false;
        for (const auto& entry : leaf_info.body) {
            EXPECT_FALSE(found_open_goal);
            found_open_goal = true;
            open_goal = entry.first;
        }
        EXPECT_TRUE(found_open_goal);

        const pud_frontier_change change = runtime.unfold(leaf, open_goal);
        live.erase(leaf);
        for (const pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
        for (const pud_global_node_id child : change.offspring)
            live.insert(child);

        pud_global_node_id successor = {};
        for (const pud_global_node_id node : change.offspring) {
            const pud_unnormalized_rule_information child_info = runtime.get_rule_information(node);
            if (!child_info.body.empty())
                successor = node;
        }
        EXPECT_FALSE(successor == pud_global_node_id{});
        leaf = successor;

        for (const pud_global_node_id node : live) {
            const pud_unnormalized_rule_information info = runtime.get_rule_information(node);
            if (!info.body.empty())
                continue;

            const expr* normalized_head =
                reference_pud_normalize_with_bindings(info.bindings, exprs, info.head);
            bool matched_depth = false;
            for (size_t depth = 0; depth <= step + 1; ++depth) {
                const expr* inner = atom("z");
                for (size_t level = 0; level < depth; ++level)
                    inner = functor("s", {inner});
                const expr* expected = functor("q", {inner});
                if (normalized_head == expected) {
                    matched_depth = true;
                    break;
                }
            }
            EXPECT_TRUE(matched_depth);
        }
    }
}
