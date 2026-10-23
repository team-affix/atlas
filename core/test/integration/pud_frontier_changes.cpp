#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_manifest_harness.hpp"

using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::UnorderedElementsAre;

namespace {

struct PudFrontierChangesIntegrationTest : public ::testing::Test {
    pud_manifest_harness h{0, pud_harness_scope::every_open_leaf};
};

}

TEST_F(PudFrontierChangesIntegrationTest, AnEmptyDatabaseOpensNothing) {
    EXPECT_EQ(h.open_axioms(), (pud_frontier_change{.opened = {}, .proven = {}, .refuted = {}}));
}

TEST_F(PudFrontierChangesIntegrationTest, FactsOpenProven) {
    h.add_colors();
    const pud_frontier_change change = h.open_axioms();
    EXPECT_THAT(change.opened, IsEmpty());
    EXPECT_THAT(change.proven, UnorderedElementsAre(0u, 1u, 2u));
    EXPECT_THAT(change.refuted, IsEmpty());
}

TEST_F(PudFrontierChangesIntegrationTest, ARuleOpensWithItsGoals) {
    h.add_colors();
    const pud_node_id rule = h.axiom(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_THAT(h.open_axioms().opened, ElementsAre(rule));
}

TEST_F(PudFrontierChangesIntegrationTest, ARuleWithAnUnprovableGoalOpensRefuted) {
    const pud_node_id rule = h.axiom(h.atom("q"), {h.atom("missing")}, 0);
    const pud_frontier_change change = h.open_axioms();
    EXPECT_THAT(change.opened, IsEmpty());
    EXPECT_THAT(change.refuted, ElementsAre(rule));
}

TEST_F(PudFrontierChangesIntegrationTest, ExpandingReportsTheChildrenButNeverTheExpandedLeaf) {
    h.add_colors();
    const pud_node_id rule = h.axiom(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("color", {h.var(0)})}, 1);
    h.open_axioms();
    const pud_frontier_change change = h.expand(rule, 0);
    EXPECT_EQ(change.opened, h.children_of(rule));
    EXPECT_THAT(change.opened, Not(Contains(rule)));
    EXPECT_THAT(change.proven, IsEmpty());
}

TEST_F(PudFrontierChangesIntegrationTest, ExpandingTheLastGoalProvesTheChildren) {
    h.add_colors();
    const pud_node_id rule = h.axiom(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    h.open_axioms();
    const pud_frontier_change change = h.expand(rule, 0);
    EXPECT_THAT(change.opened, IsEmpty());
    EXPECT_EQ(change.proven, h.children_of(rule));
}

TEST_F(PudFrontierChangesIntegrationTest, AChildRefutedOnArrivalIsReportedRefutedOnly) {
    h.add_colors();
    const pud_node_id rule = h.axiom(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    h.open_axioms();
    const pud_frontier_change change = h.expand(rule, 0);
    ASSERT_EQ(h.children_of(rule).size(), 2u);
    const pud_node_id red = h.children_of(rule).at(0);
    const pud_node_id blue = h.children_of(rule).at(1);
    EXPECT_THAT(change.opened, ElementsAre(red));
    EXPECT_THAT(change.refuted, ElementsAre(blue));
}

TEST_F(PudFrontierChangesIntegrationTest, ARefutationIsReportedInEveryTreeItReaches) {
    h.axiom(h.functor("base", {h.atom("c")}), {}, 0);
    h.axiom(h.functor("check", {h.atom("a")}), {}, 0);
    const pud_node_id mid = h.axiom(h.functor("mid", {h.var(0)}), {h.functor("base", {h.var(0)}), h.functor("check", {h.var(0)})}, 1);
    const pud_node_id top = h.axiom(h.atom("top"), {h.functor("mid", {h.atom("c")})}, 0);
    h.open_axioms();
    const pud_frontier_change change = h.expand(mid, 0);
    EXPECT_THAT(change.refuted, Contains(top));
    EXPECT_TRUE(h.refuted(mid));
}
