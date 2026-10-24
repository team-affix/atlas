#include <deque>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "reference_pud_frontier.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

// The frontier walk, stranded-node walk and node count the reload tests check runtimes against,
// checked on hand-built modules. Module 3 has axioms 0 (q :- g, h) and 1 (a fact).
struct ReferencePudFrontierTest : public ::testing::Test {
    test_functors functors;
    expr_pool exprs;
    const expr* q = exprs.make_functor(functors.id("q"), {});
    const expr* g = exprs.make_functor(functors.id("g"), {});
    const expr* h = exprs.make_functor(functors.id("h"), {});
    std::deque<pud_module> modules;
    pud_module& module = modules.emplace_back(3, std::vector<pud_module_id>{}, std::vector<rule>{rule(q, {g, h}, 0), rule(g, {}, 0)});
};

TEST_F(ReferencePudFrontierTest, UnfoldedAxiomsAreTheirOwnFrontier) {
    EXPECT_EQ(
        reference_pud_frontier(modules),
        (std::set<pud_global_node_id>{pud_global_node_id{.module = 3, .node = 0}, pud_global_node_id{.module = 3, .node = 1}}));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(reference_pud_count_nodes(module), 2u);
}

TEST_F(ReferencePudFrontierTest, AnUnfoldedNodeIsReplacedByItsChildren) {
    const pud_node_id left = module.store_inference(
        pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0});
    const pud_node_id right = module.store_inference(
        pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0});
    EXPECT_EQ(
        reference_pud_frontier(modules),
        (std::set<pud_global_node_id>{
            pud_global_node_id{.module = 3, .node = 1},
            pud_global_node_id{.module = 3, .node = left},
            pud_global_node_id{.module = 3, .node = right}}));
    EXPECT_EQ(reference_pud_count_nodes(module), 4u);
}

TEST_F(ReferencePudFrontierTest, ARefutedLeafLeavesTheFrontierButStillCounts) {
    const pud_node_id left = module.store_inference(
        pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0});
    const pud_node_id right = module.store_inference(
        pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0});
    module.mark_refuted(left);
    EXPECT_EQ(
        reference_pud_frontier(modules),
        (std::set<pud_global_node_id>{pud_global_node_id{.module = 3, .node = 1}, pud_global_node_id{.module = 3, .node = right}}));
    EXPECT_THAT(reference_pud_stranded_nodes(modules), IsEmpty());
    EXPECT_EQ(reference_pud_count_nodes(module), 4u);
}

TEST_F(ReferencePudFrontierTest, NothingBelowARefutedNodeIsOnTheFrontier) {
    const pud_node_id child = module.store_inference(
        pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0});
    module.store_inference(pud_inference{.parent = child, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0});
    module.mark_refuted(0);
    EXPECT_EQ(reference_pud_frontier(modules), (std::set<pud_global_node_id>{pud_global_node_id{.module = 3, .node = 1}}));
    EXPECT_EQ(reference_pud_count_nodes(module), 4u);
}

TEST_F(ReferencePudFrontierTest, ALiveInteriorNodeWhoseChildrenAreAllRefutedIsStranded) {
    const pud_node_id only = module.store_inference(
        pud_inference{.parent = 0, .call_site = 0, .substitutions = {}, .body_goals = {}, .var_count = 0});
    module.mark_refuted(only);
    EXPECT_THAT(reference_pud_stranded_nodes(modules), ElementsAre(pud_global_node_id{.module = 3, .node = 0}));
    EXPECT_EQ(reference_pud_frontier(modules), (std::set<pud_global_node_id>{pud_global_node_id{.module = 3, .node = 1}}));
}

TEST_F(ReferencePudFrontierTest, EveryModuleIsWalked) {
    modules.emplace_back(4, std::vector<pud_module_id>{3}, std::vector<rule>{rule(h, {}, 0)});
    EXPECT_EQ(
        reference_pud_frontier(modules),
        (std::set<pud_global_node_id>{
            pud_global_node_id{.module = 3, .node = 0},
            pud_global_node_id{.module = 3, .node = 1},
            pud_global_node_id{.module = 4, .node = 0}}));
}
