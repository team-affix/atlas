#include <optional>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_descender_system_fixture.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

struct PudDescenderAxiomHeadsIntegrationTest : public PudDescenderSystemFixture {};

TEST_F(PudDescenderAxiomHeadsIntegrationTest, AtomHeadMatchingAtomQueryClosesToEmptyNode) {
    const pud_node_id root = axiom(fn("a", {}), {}, 0);
    std::optional<pud_descent> opened = open_at_root(root, fn("a", {}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_TRUE(node_goals_.get(closed).empty());
    EXPECT_TRUE(node_specs_.get(closed).empty());
    EXPECT_EQ(node_var_counts_.get(closed), 0u);
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, AtomHeadMismatchedAtomQueryYieldsNullopt) {
    const pud_node_id root = axiom(fn("a", {}), {}, 0);
    EXPECT_FALSE(open_at_root(root, fn("b", {})).has_value());
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, AtomHeadBindsQueryVar) {
    const pud_node_id root = axiom(fn("a", {}), {}, 1);
    std::optional<pud_descent> opened = open_at_root(root, var(0));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_THAT(specs_of(closed), ElementsAre("?0=a"));
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, FactBindsAllQueryVars) {
    const expr* tom = fn("tom", {});
    const expr* bob = fn("bob", {});
    const pud_node_id root = axiom(fn("parent", {var(0), var(1)}), {}, 2);
    std::optional<pud_descent> opened = open_at_root(root, fn("parent", {var(0), var(1)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_TRUE(node_specs_.get(closed).empty());
    EXPECT_EQ(node_var_counts_.get(closed), 0u);
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, FunctorIdMismatchYieldsNullopt) {
    const pud_node_id root = axiom(fn("f", {var(0)}), {}, 1);
    EXPECT_FALSE(open_at_root(root, fn("g", {fn("a", {})})).has_value());
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, ArityMismatchYieldsNullopt) {
    const pud_node_id root = axiom(fn("f", {var(0), var(1)}), {}, 2);
    EXPECT_FALSE(open_at_root(root, fn("f", {fn("a", {})})).has_value());
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, HeadVarsGetFrameAfterCaller) {
    const pud_node_id root = axiom(fn("f", {var(0), var(1), var(2)}), {}, 3);
    pud_descent caller = descender_.descent_root(root);
    caller.lvc = 4;
    std::optional<pud_descent> opened = descender_.open_query(caller, fn("f", {fn("a", {}), var(0), var(1)}), root);
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->frame_offset, 4u);
    EXPECT_EQ(opened->lvc, 3u);
    EXPECT_EQ(opened->node, root);
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, QueryFrameOffsetFollowsCallerVarCount) {
    const pud_node_id narrow = axiom(fn("q", {}), {}, 0);
    pud_descent caller_zero = descender_.descent_root(narrow);
    std::optional<pud_descent> at_zero = descender_.open_query(caller_zero, fn("q", {}), narrow);
    ASSERT_TRUE(at_zero.has_value());
    EXPECT_EQ(at_zero->frame_offset, 0u);

    const pud_node_id wide = axiom(fn("q", {}), {}, 5);
    pud_descent caller = descender_.descent_root(wide);
    std::optional<pud_descent> at_five = descender_.open_query(caller, fn("q", {}), wide);
    ASSERT_TRUE(at_five.has_value());
    EXPECT_EQ(at_five->frame_offset, 5u);
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, VarHeadBindsHeadVarToQueryAndTouchesNothing) {
    const pud_node_id root = axiom(var(0), {}, 1);
    std::optional<pud_descent> opened = open_at_root(root, fn("q", {}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_TRUE(node_specs_.get(closed).empty());
    EXPECT_EQ(node_var_counts_.get(closed), 0u);
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, RepeatedHeadVarForcesEqualQueryArgs) {
    const pud_node_id root = axiom(fn("eq", {var(0), var(0)}), {}, 1);
    std::optional<pud_descent> opened = open_at_root(root, fn("eq", {fn("a", {}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_THAT(specs_of(closed), ElementsAre("?0=a"));
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, RepeatedHeadVarConflictYieldsNullopt) {
    const pud_node_id root = axiom(fn("eq", {var(0), var(0)}), {}, 1);
    EXPECT_FALSE(open_at_root(root, fn("eq", {fn("a", {}), fn("b", {})})).has_value());
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, NestedHeadStructureBindsDeepQueryVar) {
    const pud_node_id root = axiom(fn("pair", {fn("left", {var(0)}), var(1)}), {}, 2);
    std::optional<pud_descent> opened =
        open_at_root(root, fn("pair", {fn("left", {fn("x", {})}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    EXPECT_TRUE(expr_eq(value_of(*opened, 0), fn("x", {})));
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, ListHeadDecomposesGroundQueryList) {
    const expr* ground = list({fn("a", {}), fn("b", {})});
    const pud_node_id root = axiom(fn("same", {var(0)}), {}, 1);
    std::optional<pud_descent> opened = open_at_root(root, fn("same", {ground}));
    ASSERT_TRUE(opened.has_value());
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, HeadVarBoundToQueryStructureClosesToFreshVar) {
    const expr* ground = list({fn("a", {})});
    const pud_node_id root = axiom(var(0), {}, 1);
    std::optional<pud_descent> opened = open_at_root(root, ground);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(node_var_counts_.get(closed), 0u);
    EXPECT_TRUE(node_specs_.get(closed).empty());
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, RootIdSelectsWhichHeadIsUsed) {
    const pud_node_id accepts = axiom(fn("f", {}), {}, 0);
    const pud_node_id rejects = axiom(fn("g", {}), {}, 0);
    EXPECT_TRUE(open_at_root(accepts, fn("f", {})).has_value());
    EXPECT_FALSE(open_at_root(rejects, fn("f", {})).has_value());
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, OpenQueryCallerRepsSurfaceOnClose) {
    const pud_node_id main_id = axiom(
        fn("main", {var(0), var(1), var(2)}),
        {fn("parent", {var(0), var(2)})},
        3);
    const pud_node_id fact = axiom(fn("parent", {fn("tom", {}), fn("bob", {})}), {}, 0);
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* const* goal = at_main.pending_body_goals.find(0);
    ASSERT_TRUE(goal);
    std::optional<pud_descent> opened = descender_.open_query(at_main, *goal, fact);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_THAT(specs_of(closed), ElementsAre("?0=tom", "?2=bob"));
}

TEST_F(PudDescenderAxiomHeadsIntegrationTest, OpenedDescentCarriesAxiomBodyGoalsAndRootNode) {
    const expr* goal_a = fn("goal-a", {});
    const expr* goal_b = fn("goal-b", {});
    const pud_node_id root = axiom(fn("main", {var(0)}), {goal_a, goal_b}, 1);
    std::optional<pud_descent> opened = open_at_root(root, fn("main", {fn("x", {})}));
    ASSERT_TRUE(opened.has_value());
    EXPECT_EQ(opened->node, root);
    EXPECT_EQ(opened->pending_body_goals.size(), 2u);
    EXPECT_TRUE(expr_eq(*opened->pending_body_goals.find(0), goal_a));
    EXPECT_TRUE(expr_eq(*opened->pending_body_goals.find(1), goal_b));
}
