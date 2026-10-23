#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_frontier_test_fixture.hpp"

using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::UnorderedElementsAre;

namespace {

struct PudFrontierChangesIntegrationTest : public ::testing::Test {
    pud_frontier_test_fixture h;
};

}

TEST_F(PudFrontierChangesIntegrationTest, AnEmptyModuleOpensNothing) {
    EXPECT_EQ(h.register_module(), (pud_frontier_change{.offspring = {}, .refuted = {}}));
}

TEST_F(PudFrontierChangesIntegrationTest, FactsReportAsOffspring) {
    const std::vector<pud_global_node_id> colors = h.add_colors();
    const pud_frontier_change change = h.register_module();
    EXPECT_THAT(change.offspring, ElementsAreArray(colors));
    EXPECT_THAT(change.refuted, IsEmpty());
}

TEST_F(PudFrontierChangesIntegrationTest, ARuleOpensWithItsGoals) {
    const std::vector<pud_global_node_id> colors = h.add_colors();
    const pud_global_node_id rule = h.axiom(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_THAT(h.register_module().offspring, UnorderedElementsAre(colors.at(0), colors.at(1), colors.at(2), rule));
}

TEST_F(PudFrontierChangesIntegrationTest, ARuleWithAnUnprovableGoalOpensRefuted) {
    const pud_global_node_id rule = h.axiom(h.atom("q"), {h.atom("missing")}, 0);
    const pud_frontier_change change = h.register_module();
    EXPECT_THAT(change.offspring, IsEmpty());
    EXPECT_THAT(change.refuted, ElementsAre(rule));
    EXPECT_TRUE(h.refuted(rule));
}

TEST_F(PudFrontierChangesIntegrationTest, ExpandingReportsTheChildrenButNeverTheExpandedLeaf) {
    h.add_colors();
    const pud_global_node_id rule =
        h.axiom(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("color", {h.var(0)})}, 1);
    h.register_module();
    const pud_frontier_change change = h.expand(rule, 0);
    EXPECT_EQ(change.offspring, h.children_of(rule));
    EXPECT_THAT(change.offspring, Not(Contains(rule)));
}

TEST_F(PudFrontierChangesIntegrationTest, ExpandingTheLastGoalLeavesGoallessChildrenInOffspring) {
    h.add_colors();
    const pud_global_node_id rule = h.axiom(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    h.register_module();
    const pud_frontier_change change = h.expand(rule, 0);
    EXPECT_EQ(change.offspring, h.children_of(rule));
}

TEST_F(PudFrontierChangesIntegrationTest, ChildrenAreStoredInTheExpandedLeafsModule) {
    h.add_colors();
    const pud_global_node_id rule = h.axiom(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    h.register_module();
    for (pud_global_node_id child : h.expand(rule, 0).offspring)
        EXPECT_EQ(child.module, rule.module);
}

TEST_F(PudFrontierChangesIntegrationTest, AChildRefutedOnArrivalIsReportedRefutedOnly) {
    h.add_colors();
    const pud_global_node_id rule =
        h.axiom(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    h.register_module();
    const pud_frontier_change change = h.expand(rule, 0);
    ASSERT_EQ(h.children_of(rule).size(), 2u);
    const pud_global_node_id red = h.children_of(rule).at(0);
    const pud_global_node_id blue = h.children_of(rule).at(1);
    EXPECT_THAT(change.offspring, ElementsAre(red));
    EXPECT_THAT(change.refuted, ElementsAre(blue));
    EXPECT_TRUE(h.refuted(blue));
    EXPECT_FALSE(h.refuted(red));
}

TEST_F(PudFrontierChangesIntegrationTest, SiblingsRefutedOnArrivalThatShareAnExhaustedQueryAreRefutedOnce) {
    h.axiom(h.functor("q", {h.var(0)}), {h.functor("r", {h.var(0)})}, 1);
    h.axiom(h.functor("q", {h.var(0)}), {h.functor("s", {h.var(0)})}, 1);
    h.axiom(h.functor("r", {h.atom("c")}), {}, 0);
    h.axiom(h.functor("s", {h.atom("c")}), {}, 0);
    const pud_global_node_id rule = h.axiom(h.atom("p"), {h.atom("p"), h.functor("q", {h.atom("d")})}, 0);
    h.register_module();
    const pud_frontier_change change = h.expand(rule, 1);
    ASSERT_EQ(h.children_of(rule).size(), 2u);
    EXPECT_THAT(change.refuted, UnorderedElementsAre(h.children_of(rule).at(0), h.children_of(rule).at(1)));
    EXPECT_THAT(change.offspring, IsEmpty());
}

TEST_F(PudFrontierChangesIntegrationTest, ARefutationIsReportedInEveryTreeItReaches) {
    h.axiom(h.functor("base", {h.atom("c")}), {}, 0);
    h.axiom(h.functor("check", {h.atom("a")}), {}, 0);
    const pud_global_node_id mid =
        h.axiom(h.functor("mid", {h.var(0)}), {h.functor("base", {h.var(0)}), h.functor("check", {h.var(0)})}, 1);
    const pud_global_node_id top = h.axiom(h.atom("top"), {h.functor("mid", {h.atom("c")})}, 0);
    h.register_module();
    const pud_frontier_change change = h.expand(mid, 0);
    EXPECT_THAT(change.refuted, Contains(top));
    EXPECT_TRUE(h.refuted(mid));
    EXPECT_TRUE(h.refuted(top));
}

TEST_F(PudFrontierChangesIntegrationTest, AnAxiomWhoseOnlyCandidateIsRefutedDuringRegistrationIsRefutedToo) {
    h.add_colors();
    const pud_global_node_id rule =
        h.axiom(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)}), h.atom("never")}, 1);
    h.axiom(h.atom("never"), {h.atom("never_holds")}, 0);
    const pud_frontier_change registered = h.register_module();
    EXPECT_THAT(registered.refuted, Contains(rule));
    EXPECT_TRUE(h.refuted(rule));
}
