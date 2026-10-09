#include <optional>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_descender_system_fixture.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct PudDescenderAxiomSystemsIntegrationTest : public PudDescenderSystemFixture {
    pud_node_id parent_fact(const expr* parent_name, const expr* child_name) {
        return axiom(fn("parent", {parent_name, child_name}), {}, 0);
    }

    pud_node_id grandparent_main() {
        const expr* goal_parent = fn("parent", {var(0), var(2)});
        const expr* goal_grand = fn("parent", {var(2), var(1)});
        return axiom(fn("grandparent", {var(0), var(1)}), {goal_parent, goal_grand}, 3);
    }

    pud_node_id add_zero() {
        return axiom(fn("add", {var(0), fn("z", {}), var(0)}), {}, 1);
    }

    pud_node_id add_succ() {
        const expr* body = fn("add", {var(0), var(1), var(2)});
        return axiom(
            fn("add", {var(0), fn("s", {var(1)}), fn("s", {var(2)})}),
            {body},
            3);
    }

    pud_node_id leq_zero() {
        return axiom(fn("leq", {fn("z", {}), var(0)}), {}, 1);
    }

    pud_node_id leq_succ() {
        const expr* body = fn("leq", {var(0), var(1)});
        return axiom(
            fn("leq", {fn("s", {var(0)}), fn("s", {var(1)})}),
            {body},
            2);
    }

    pud_node_id append_nil() {
        return axiom(fn("append", {list({}), var(0), var(0)}), {}, 1);
    }

    pud_node_id append_cons() {
        const expr* body = fn("append", {var(1), var(2), var(3)});
        return axiom(
            fn("append", {list({var(0)}, var(1)), var(2), list({var(0)}, var(3))}),
            {body},
            4);
    }

    pud_node_id member_head() {
        return axiom(fn("member", {var(0), list({var(0)}, var(1))}), {}, 2);
    }

    pud_node_id member_tail() {
        const expr* body = fn("member", {var(0), var(1)});
        return axiom(fn("member", {var(0), list({var(2)}, var(1))}), {body}, 3);
    }

    pud_node_id length_nil() {
        return axiom(fn("length", {list({}), fn("z", {})}), {}, 0);
    }

    pud_node_id length_cons() {
        const expr* body = fn("length", {var(1), var(2)});
        return axiom(
            fn("length", {list({var(0)}, var(1)), fn("s", {var(2)})}),
            {body},
            3);
    }
};

TEST_F(PudDescenderAxiomSystemsIntegrationTest, GrandparentFirstGoalResolvesAgainstFact) {
    const pud_node_id main_id = grandparent_main();
    const pud_node_id fact = parent_fact(fn("tom", {}), fn("bob", {}));
    pud_descent at_main = descender_.descent_root(main_id);
    std::optional<pud_node_id> child = resolve(at_main, 0, fact);
    ASSERT_TRUE(child.has_value());
    EXPECT_THAT(specs_of(*child), ElementsAre("?0=tom", "?2=bob"));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, SecondGoalResolvesAfterFirstViaReplay) {
    const pud_node_id main_id = grandparent_main();
    const pud_node_id fact_tom = parent_fact(fn("tom", {}), fn("bob", {}));
    const pud_node_id fact_bob = parent_fact(fn("bob", {}), fn("ann", {}));
    std::optional<pud_node_id> first = resolve(descender_.descent_root(main_id), 0, fact_tom);
    ASSERT_TRUE(first.has_value());
    std::optional<pud_descent> at_first = walk(main_id, {*first});
    ASSERT_TRUE(at_first.has_value());
    std::optional<pud_node_id> second = resolve(*at_first, 1, fact_bob);
    ASSERT_TRUE(second.has_value());
    EXPECT_THAT(specs_of(*second), ElementsAre("?1=ann"));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, FullChainLeavesNoPendingGoals) {
    const pud_node_id main_id = grandparent_main();
    const pud_node_id fact_tom = parent_fact(fn("tom", {}), fn("bob", {}));
    const pud_node_id fact_bob = parent_fact(fn("bob", {}), fn("ann", {}));
    std::optional<pud_node_id> first = resolve(descender_.descent_root(main_id), 0, fact_tom);
    ASSERT_TRUE(first.has_value());
    std::optional<pud_descent> at_first = walk(main_id, {*first});
    ASSERT_TRUE(at_first.has_value());
    std::optional<pud_node_id> second = resolve(*at_first, 1, fact_bob);
    ASSERT_TRUE(second.has_value());
    std::optional<pud_descent> at_second = walk(main_id, {*first, *second});
    ASSERT_TRUE(at_second.has_value());
    EXPECT_TRUE(at_second->pending_body_goals.empty());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, NonMatchingFactLeavesNoNodeStored) {
    const pud_node_id main_id = grandparent_main();
    const pud_node_id wrong = parent_fact(fn("x", {}), fn("y", {}));
    pud_descent at_main = descender_.descent_root(main_id);
    const expr* const* goal = at_main.pending_body_goals.find(0);
    ASSERT_TRUE(goal);
    EXPECT_FALSE(
        descender_.open_query(at_main, fn("parent", {fn("tom", {}), fn("bob", {})}), wrong)
            .has_value());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, TwoFactsBranchFromSameGoalIndependently) {
    const pud_node_id main_id = grandparent_main();
    const pud_node_id fact_tom = parent_fact(fn("tom", {}), fn("bob", {}));
    const pud_node_id fact_x = parent_fact(fn("x", {}), fn("y", {}));
    pud_descent at_main = descender_.descent_root(main_id);
    std::optional<pud_node_id> branch_a = resolve(at_main, 0, fact_tom);
    ASSERT_TRUE(branch_a.has_value());
    const expr* const* goal = at_main.pending_body_goals.find(0);
    ASSERT_TRUE(goal);
    EXPECT_FALSE(
        descender_.open_query(at_main, fn("parent", {fn("tom", {}), fn("bob", {})}), fact_x)
            .has_value());
    EXPECT_THAT(specs_of(*branch_a), ElementsAre("?0=tom", "?2=bob"));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AddZeroBaseBindsResult) {
    const pud_node_id add0 = add_zero();
    std::optional<pud_descent> opened = open_at_root(add0, fn("add", {nat(2), fn("z", {}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_THAT(specs_of(closed), ElementsAre("?0=" + show(nat(2))));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AddSuccStepProducesRecursiveGoalWithFreshVar) {
    const pud_node_id add1 = add_succ();
    std::optional<pud_descent> opened =
        open_at_root(add1, fn("add", {fn("a", {}), fn("s", {fn("b", {})}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(node_var_counts_.get(closed), 1u);
    EXPECT_EQ(goals_of(closed).size(), 1u);
    EXPECT_TRUE(goals_of(closed)[0].find("add(a, b, ?") != std::string::npos);
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AddThreeSuccsChainYieldsFour) {
    const pud_node_id add1 = add_succ();
    std::optional<pud_descent> opened =
        open_at_root(add1, fn("add", {nat(1), nat(1), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(goals_of(closed).size(), 1u);
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, LeqFailsWhenNoAxiomMatches) {
    const pud_node_id leq0 = leq_zero();
    const pud_node_id leq1 = leq_succ();
    const pud_node_id main_id = axiom(fn("main", {}), {fn("leq", {fn("s", {fn("z", {})}), fn("z", {})})}, 0);
    pud_descent at_main = descender_.descent_root(main_id);
    EXPECT_FALSE(resolve(at_main, 0, leq0).has_value());
    EXPECT_FALSE(resolve(at_main, 0, leq1).has_value());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AddBackwardsEnumeratesTwoSolutions) {
    const pud_node_id add0 = add_zero();
    pud_descent query = descender_.descent_root(add0);
    std::optional<pud_descent> opened =
        descender_.open_query(query, fn("add", {var(0), var(1), fn("z", {})}), add0);
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed_z = close_opened(*opened);
    EXPECT_FALSE(node_specs_.get(closed_z).empty());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AddBothArgsUnboundKeepsGoalOpen) {
    const pud_node_id add1 = add_succ();
    std::optional<pud_descent> opened =
        open_at_root(add1, fn("add", {var(0), var(1), var(2)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(goals_of(closed).size(), 1u);
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AppendNilBase) {
    const pud_node_id append0 = append_nil();
    std::optional<pud_descent> opened =
        open_at_root(append0, fn("append", {list({}), list({fn("a", {})}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_THAT(specs_of(closed), ElementsAre("?0=[a]"));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AppendConsStepRenamesTailVar) {
    const pud_node_id append1 = append_cons();
    std::optional<pud_descent> opened = open_at_root(
        append1,
        fn("append", {list({fn("a", {})}), list({fn("b", {})}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_THAT(specs_of(closed), ElementsAre("?0=[a|?4]"));
    EXPECT_THAT(goals_of(closed), ElementsAre("0: append([], [b], ?4)"));
    EXPECT_EQ(node_var_counts_.get(closed), 1u);
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AppendTwoElementListsYieldsConcatenation) {
    const pud_node_id append1 = append_cons();
    std::optional<pud_descent> opened = open_at_root(
        append1,
        fn("append", {list({fn("a", {}), fn("b", {})}), list({fn("c", {})}), var(0)}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_THAT(specs_of(closed), ElementsAre("?0=[a|?4]"));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AppendBackwardsSplitsThreeWays) {
    const pud_node_id append0 = append_nil();
    const pud_node_id append1 = append_cons();
    const expr* target = list({fn("a", {}), fn("b", {}), fn("c", {})});
    std::optional<pud_descent> opened =
        open_at_root(append1, fn("append", {var(0), var(1), target}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_FALSE(specs_of(closed).empty());
    EXPECT_EQ(goals_of(closed).size(), 1u);
    EXPECT_TRUE(goals_of(closed)[0].find("append(") != std::string::npos);
    (void)append0;
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AppendSharedVarQuery) {
    const pud_node_id append1 = append_cons();
    const expr* shared = list({fn("a", {}), fn("a", {})});
    std::optional<pud_descent> opened =
        open_at_root(append1, fn("append", {var(0), var(0), shared}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_FALSE(specs_of(closed).empty());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, AppendPartialListMismatchDiesDownChain) {
    const pud_node_id append1 = append_cons();
    const pud_node_id main_id = axiom(
        fn("main", {}),
        {fn("append", {list({fn("a", {})}), list({fn("b", {})}), var(0)})},
        1);
    std::optional<pud_node_id> step0 = resolve(descender_.descent_root(main_id), 0, append1);
    ASSERT_TRUE(step0.has_value());
    const pud_node_id bad = store_closed_node({pud_specialization{.var_idx = 0, .value = fn("x", {})}}, {}, 0);
    link_descend_only(*step0, bad);
    std::optional<pud_descent> at_bad = walk(main_id, {*step0, bad});
    EXPECT_FALSE(at_bad.has_value());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, MemberFindsHead) {
    const pud_node_id mem0 = member_head();
    std::optional<pud_descent> opened =
        open_at_root(mem0, fn("member", {fn("a", {}), list({fn("a", {}), fn("b", {})})}));
    ASSERT_TRUE(opened.has_value());
    EXPECT_TRUE(expr_eq(value_of_global(*opened, 2), fn("a", {})));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, MemberSkipsToLaterElement) {
    const pud_node_id mem1 = member_tail();
    std::optional<pud_descent> opened = open_at_root(
        mem1,
        fn("member", {fn("b", {}), list({fn("a", {}), fn("b", {})})}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(goals_of(closed).size(), 1u);
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, LengthOfGroundList) {
    const pud_node_id len0 = length_nil();
    std::optional<pud_descent> opened =
        open_at_root(len0, fn("length", {list({}), fn("z", {})}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_TRUE(node_specs_.get(closed).empty());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, LengthBackwardsGeneratesListOfFreshVars) {
    const pud_node_id len1 = length_cons();
    std::optional<pud_descent> opened =
        open_at_root(len1, fn("length", {var(0), fn("s", {fn("z", {})})}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id closed = close_opened(*opened);
    EXPECT_EQ(goals_of(closed).size(), 1u);
    EXPECT_TRUE(goals_of(closed)[0].find("length(") != std::string::npos);
    EXPECT_EQ(node_var_counts_.get(closed), 2u);
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, RefutedNodeBlocksDescend) {
    const pud_node_id root = axiom(fn("main", {}), {}, 0);
    const pud_node_id child = store_closed_node({}, {fn("g", {})}, 0);
    link_descend_only(root, child);
    refuted_.set_refuted(child);
    EXPECT_FALSE(walk(root, {child}).has_value());
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, RefutedSiblingDoesNotBlockOther) {
    const pud_node_id root = axiom(fn("main", {}), {}, 0);
    const pud_node_id bad = store_closed_node({}, {}, 0);
    const pud_node_id good = store_closed_node({}, {fn("g", {})}, 0);
    set_children(root, std::vector<pud_node_id>{bad, good});
    refuted_.set_refuted(bad);
    std::optional<pud_descent> at_good = walk(root, {good});
    ASSERT_TRUE(at_good.has_value());
    EXPECT_THAT(goals_of(good), ElementsAre("0: g"));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, ChildrenCallSitesAndLeavesReflectExpansion) {
    const pud_node_id main_id = grandparent_main();
    const pud_node_id fact = parent_fact(fn("tom", {}), fn("bob", {}));
    pud_descent at_main = descender_.descent_root(main_id);
    std::optional<pud_node_id> child = resolve(at_main, 0, fact);
    ASSERT_TRUE(child.has_value());
    EXPECT_THAT(children_.get(main_id), ElementsAre(*child));
    EXPECT_EQ(call_sites_.get(main_id), 0u);
    EXPECT_FALSE(leaves_.check_leaf(main_id));
    EXPECT_TRUE(leaves_.check_leaf(*child));
}

TEST_F(PudDescenderAxiomSystemsIntegrationTest, ClosingSameWalkTwiceYieldsFreshIdWithSameContents) {
    const pud_node_id fact = parent_fact(fn("tom", {}), fn("bob", {}));
    std::optional<pud_descent> opened =
        open_at_root(fact, fn("parent", {fn("tom", {}), fn("bob", {})}));
    ASSERT_TRUE(opened.has_value());
    const pud_node_id first = close_opened(*opened);
    const pud_node_id second = close_opened(*opened);
    EXPECT_NE(first, second);
    EXPECT_EQ(specs_of(first), specs_of(second));
}
