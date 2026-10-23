#if 0 // Disabled: depended on pud_solver and pud_manifest_harness.
#include <algorithm>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_manifest_harness.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct PudManifestConjectureIntegrationTest : public ::testing::Test {
    pud_manifest_harness h{0, pud_harness_scope::conjecture_leaves};
};

TEST_F(PudManifestConjectureIntegrationTest, GroundQueryAgainstAFactIsSolved) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("color", {h.atom("red")})}, 0);
    EXPECT_THAT(h.enumerate(conjecture, 10, 50), ElementsAre(h.atom("q")));
}

TEST_F(PudManifestConjectureIntegrationTest, QueryVariableIsBoundByEachMatchingFact) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_THAT(
        h.enumerate(conjecture, 10, 50),
        UnorderedElementsAre(h.functor("q", {h.atom("red")}), h.functor("q", {h.atom("blue")})));
    EXPECT_TRUE(h.exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, QueryWithNoMatchingAxiomIsRefutedAtStart) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("color", {h.atom("green")})}, 0);
    EXPECT_TRUE(h.refuted(conjecture));
    EXPECT_FALSE(h.next_solution(conjecture, 50).has_value());
}

TEST_F(PudManifestConjectureIntegrationTest, QueryWithNoBodyGoalsIsItsOwnSolution) {
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {}, 0);
    const std::optional<pud_solution> solution = h.next_solution(conjecture, 1);
    ASSERT_TRUE(solution.has_value());
    EXPECT_EQ(solution->leaf, conjecture);
}

TEST_F(PudManifestConjectureIntegrationTest, ConjunctionKeepsOnlyBindingsThatSatisfyEveryGoal) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(
        h.functor("q", {h.var(0)}),
        {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})},
        1);
    EXPECT_THAT(h.enumerate(conjecture, 10, 50), ElementsAre(h.functor("q", {h.atom("red")})));
    EXPECT_TRUE(h.exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, ConjunctionWithNoCommonBindingIsRefuted) {
    h.add_colors();
    h.axiom(h.functor("cold", {h.atom("blue")}), {}, 0);
    const pud_node_id conjecture = h.start_conjecture(
        h.atom("q"),
        {h.functor("warm", {h.var(0)}), h.functor("cold", {h.var(0)})},
        1);
    EXPECT_THAT(h.enumerate(conjecture, 10, 50), IsEmpty());
    EXPECT_TRUE(h.refuted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, NaturalNumbersAreEnumeratedInOrder) {
    h.add_nat();
    const pud_node_id conjecture = h.start_conjecture(h.functor("q", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    EXPECT_THAT(
        h.enumerate(conjecture, 5, 100),
        ElementsAre(
            h.functor("q", {h.peano(0)}),
            h.functor("q", {h.peano(1)}),
            h.functor("q", {h.peano(2)}),
            h.functor("q", {h.peano(3)}),
            h.functor("q", {h.peano(4)})));
    EXPECT_FALSE(h.refuted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, GroundNaturalNumberIsChecked) {
    h.add_nat();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("nat", {h.peano(3)})}, 0);
    EXPECT_THAT(h.enumerate(conjecture, 10, 100), ElementsAre(h.atom("q")));
    EXPECT_TRUE(h.exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, NonNaturalIsRefutedAfterUnfolding) {
    h.add_nat();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("nat", {h.functor("s", {h.atom("bad")})})}, 0);
    EXPECT_THAT(h.enumerate(conjecture, 10, 100), IsEmpty());
    EXPECT_TRUE(h.refuted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, AppendBuildsTheConcatenation) {
    h.add_append();
    const pud_node_id conjecture = h.start_conjecture(
        h.functor("q", {h.var(0)}),
        {h.functor("app", {h.list({h.atom("a")}), h.list({h.atom("b"), h.atom("c")}), h.var(0)})},
        1);
    EXPECT_THAT(h.enumerate(conjecture, 10, 100), ElementsAre(h.functor("q", {h.list({h.atom("a"), h.atom("b"), h.atom("c")})})));
    EXPECT_TRUE(h.exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, AppendSplitsAListEveryWay) {
    h.add_append();
    const pud_node_id conjecture = h.start_conjecture(
        h.functor("q", {h.var(0), h.var(1)}),
        {h.functor("app", {h.var(0), h.var(1), h.list({h.atom("a"), h.atom("b")})})},
        2);
    EXPECT_THAT(
        h.enumerate(conjecture, 10, 100),
        UnorderedElementsAre(
            h.functor("q", {h.list({}), h.list({h.atom("a"), h.atom("b")})}),
            h.functor("q", {h.list({h.atom("a")}), h.list({h.atom("b")})}),
            h.functor("q", {h.list({h.atom("a"), h.atom("b")}), h.list({})})));
    EXPECT_TRUE(h.exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, MemberFindsEveryElement) {
    h.add_member();
    const pud_node_id conjecture = h.start_conjecture(
        h.functor("q", {h.var(0)}),
        {h.functor("mem", {h.var(0), h.list({h.atom("a"), h.atom("b"), h.atom("c")})})},
        1);
    EXPECT_THAT(
        h.enumerate(conjecture, 10, 100),
        UnorderedElementsAre(h.functor("q", {h.atom("a")}), h.functor("q", {h.atom("b")}), h.functor("q", {h.atom("c")})));
    EXPECT_TRUE(h.exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, PeanoAdditionIsInverted) {
    h.add_peano_add();
    const pud_node_id conjecture = h.start_conjecture(
        h.functor("q", {h.var(0), h.var(1)}),
        {h.functor("add", {h.var(0), h.var(1), h.peano(2)})},
        2);
    EXPECT_THAT(
        h.enumerate(conjecture, 10, 100),
        UnorderedElementsAre(
            h.functor("q", {h.peano(0), h.peano(2)}),
            h.functor("q", {h.peano(1), h.peano(1)}),
            h.functor("q", {h.peano(2), h.peano(0)})));
    EXPECT_TRUE(h.exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, TransitiveClosureTerminatesOnAFiniteGraph) {
    h.axiom(h.functor("parent", {h.atom("a"), h.atom("b")}), {}, 0);
    h.axiom(h.functor("parent", {h.atom("b"), h.atom("c")}), {}, 0);
    h.axiom(h.functor("anc", {h.var(0), h.var(1)}), {h.functor("parent", {h.var(0), h.var(1)})}, 2);
    h.axiom(
        h.functor("anc", {h.var(0), h.var(1)}),
        {h.functor("parent", {h.var(0), h.var(2)}), h.functor("anc", {h.var(2), h.var(1)})},
        3);
    const pud_node_id conjecture = h.start_conjecture(h.functor("q", {h.var(0)}), {h.functor("anc", {h.atom("a"), h.var(0)})}, 1);
    EXPECT_THAT(
        h.enumerate(conjecture, 10, 200),
        UnorderedElementsAre(h.functor("q", {h.atom("b")}), h.functor("q", {h.atom("c")})));
    EXPECT_TRUE(h.exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, SharedVariablesAcrossGoalsAreRespected) {
    h.add_append();
    h.add_member();
    const pud_node_id conjecture = h.start_conjecture(
        h.functor("q", {h.var(0)}),
        {
            h.functor("mem", {h.var(0), h.list({h.atom("a"), h.atom("b"), h.atom("c")})}),
            h.functor("app", {h.list({h.atom("x")}), h.list({h.var(0)}), h.list({h.atom("x"), h.atom("b")})}),
        },
        1);
    EXPECT_THAT(h.enumerate(conjecture, 10, 200), ElementsAre(h.functor("q", {h.atom("b")})));
    EXPECT_TRUE(h.exhausted(conjecture));
}

TEST_F(PudManifestConjectureIntegrationTest, ExhaustedBudgetLeavesTheQueryLive) {
    h.add_nat();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("nat", {h.peano(5)})}, 0);
    EXPECT_FALSE(h.next_solution(conjecture, 2).has_value());
    EXPECT_FALSE(h.refuted(conjecture));
    EXPECT_TRUE(h.next_solution(conjecture, 100).has_value());
}

TEST_F(PudManifestConjectureIntegrationTest, RefutingOneSolutionKeepsTheOthers) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    const std::optional<pud_solution> first = h.next_solution(conjecture, 50);
    ASSERT_TRUE(first.has_value());
    h.refute(first->leaf);
    EXPECT_TRUE(h.refuted(first->leaf));
    EXPECT_FALSE(h.refuted(conjecture));
    const std::optional<pud_solution> second = h.next_solution(conjecture, 50);
    ASSERT_TRUE(second.has_value());
    EXPECT_NE(second->leaf, first->leaf);
}

TEST_F(PudManifestConjectureIntegrationTest, TwoQueriesShareTheDatabase) {
    h.add_colors();
    const pud_node_id warm_conjecture = h.start_conjecture(h.functor("w", {h.var(0)}), {h.functor("warm", {h.var(0)})}, 1);
    const pud_node_id color_conjecture = h.start_conjecture(h.functor("c", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_THAT(h.enumerate(warm_conjecture, 10, 50), ElementsAre(h.functor("w", {h.atom("red")})));
    EXPECT_THAT(
        h.enumerate(color_conjecture, 10, 50),
        UnorderedElementsAre(h.functor("c", {h.atom("red")}), h.functor("c", {h.atom("blue")})));
}

TEST_F(PudManifestConjectureIntegrationTest, SolutionLeafHasNoOpenGoals) {
    h.add_nat();
    const pud_node_id conjecture = h.start_conjecture(h.functor("q", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    const std::optional<pud_solution> solution = h.next_solution(conjecture, 50);
    ASSERT_TRUE(solution.has_value());
    EXPECT_TRUE(h.open_goals(solution->leaf).empty());
    EXPECT_TRUE(h.live_leaf(solution->leaf));
}

TEST_F(PudManifestConjectureIntegrationTest, ManyNaturalNumbersStayCorrect) {
    h.add_nat();
    const pud_node_id conjecture = h.start_conjecture(h.functor("q", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    const std::vector<const expr*> answers = h.enumerate(conjecture, 12, 500);
    ASSERT_EQ(answers.size(), 12u);
    for (uint32_t count = 0; count < 12; ++count)
        EXPECT_EQ(answers.at(count), h.functor("q", {h.peano(count)}));
}

TEST_F(PudManifestConjectureIntegrationTest, SolvingLeavesOnlyTheFrontiersQueriesAlive) {
    h.add_nat();
    const pud_node_id conjecture = h.start_conjecture(h.functor("q", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    const pud_mhq_head_id root_query = h.query_of(conjecture, 0).value();
    ASSERT_TRUE(h.next_solution(conjecture, 20).has_value());
    EXPECT_FALSE(h.alive(root_query));
    for (pud_node_id leaf : h.live_leaves()) {
        if (h.root_of(leaf) != conjecture)
            continue;
        for (const pud_body_goal& goal : h.open_goals(leaf))
            EXPECT_TRUE(h.alive(h.query_of(leaf, goal.id).value()));
    }
}

struct PudManifestConjectureLemmaIntegrationTest : public ::testing::Test {
    pud_manifest_harness h{0, pud_harness_scope::conjecture_leaves};
    pud_node_id nat_fact = 0;
    pud_node_id nat_rule = 0;

    void add_nat_tracked() {
        nat_fact = h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
        nat_rule = h.axiom(h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
    }

    std::vector<pud_node_id> open_children(pud_node_id node) {
        std::vector<pud_node_id> open;
        for (pud_node_id child : h.children_of(node)) {
            if (h.open(child))
                open.push_back(child);
        }
        return open;
    }

    pud_node_id grow_nat_rule(uint32_t depth) {
        h.open_axioms();
        pud_node_id frontier = nat_rule;
        for (uint32_t level = 0; level < depth; ++level) {
            h.expand(frontier, h.first_open_goal(frontier));
            const std::vector<pud_node_id> recursive = open_children(frontier);
            EXPECT_EQ(recursive.size(), 1u);
            frontier = recursive.front();
        }
        return frontier;
    }

    bool solvable(const expr* goal) {
        const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {goal}, 0);
        return h.next_solution(conjecture, 200).has_value();
    }
};

TEST_F(PudManifestConjectureLemmaIntegrationTest, ExpandingTheRuleTreeGivesAFactAndARecursiveChild) {
    add_nat_tracked();
    h.open_axioms();
    h.expand(nat_rule, 0);
    EXPECT_FALSE(h.leaf(nat_rule));
    EXPECT_EQ(h.children_of(nat_rule).size(), 2u);
    EXPECT_EQ(open_children(nat_rule).size(), 1u);
    EXPECT_FALSE(h.refuted(nat_rule));
}

TEST_F(PudManifestConjectureLemmaIntegrationTest, GroundNumbersAreStillSolvedOnceTheRuleTreeHasGrown) {
    add_nat_tracked();
    grow_nat_rule(3);
    for (uint32_t count = 0; count <= 6; ++count)
        EXPECT_TRUE(solvable(h.functor("nat", {h.peano(count)}))) << count;
}

TEST_F(PudManifestConjectureLemmaIntegrationTest, NonNumbersAreStillRefutedOnceTheRuleTreeHasGrown) {
    add_nat_tracked();
    grow_nat_rule(3);
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("nat", {h.peano_on(2, "bad")})}, 0);
    EXPECT_FALSE(h.next_solution(conjecture, 200).has_value());
    EXPECT_TRUE(h.refuted(conjecture));
}

TEST_F(PudManifestConjectureLemmaIntegrationTest, EnumerationIsUnchangedByGrowingTheRuleTree) {
    add_nat_tracked();
    grow_nat_rule(2);
    const pud_node_id conjecture = h.start_conjecture(h.functor("q", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    const std::vector<const expr*> answers = h.enumerate(conjecture, 6, 300);
    ASSERT_EQ(answers.size(), 6u);
    for (const expr* found : answers) {
        bool is_number = false;
        for (uint32_t count = 0; count < 20; ++count)
            is_number = is_number || found == h.functor("q", {h.peano(count)});
        EXPECT_TRUE(is_number);
    }
    std::vector<const expr*> distinct = answers;
    std::sort(distinct.begin(), distinct.end());
    EXPECT_EQ(std::unique(distinct.begin(), distinct.end()), distinct.end());
}

TEST_F(PudManifestConjectureLemmaIntegrationTest, DeepLemmaResolvesALargeNumberInFewExpansions) {
    add_nat_tracked();
    grow_nat_rule(6);
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("nat", {h.peano(6)})}, 0);
    EXPECT_TRUE(h.next_solution(conjecture, 3).has_value());
}

TEST_F(PudManifestConjectureLemmaIntegrationTest, RefutingTheRuleTreesRecursiveBranchKeepsQueriesCorrect) {
    add_nat_tracked();
    const pud_node_id recursive = grow_nat_rule(2);
    h.refute(recursive);
    EXPECT_TRUE(h.refuted(recursive));
    EXPECT_FALSE(h.refuted(nat_rule));
    EXPECT_TRUE(solvable(h.functor("nat", {h.peano(1)})));
}

// Expands and refutes conjecture leaves by hand to watch when their queries are let go of.
struct PudManifestConjectureQueryLifetimeIntegrationTest : public ::testing::Test {
    pud_manifest_harness h{0, pud_harness_scope::every_open_leaf};
};

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, RootsQueriesLiveWhileTheRootIsALeaf) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(1)})}, 2);
    EXPECT_TRUE(h.alive(h.query_of(conjecture, 0).value()));
    EXPECT_TRUE(h.alive(h.query_of(conjecture, 1).value()));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, AnExpandedLeafLeavesTheFrontierToItsChildren) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("color", {h.var(0)})}, 1);
    h.expand(conjecture, 0);
    EXPECT_FALSE(h.live_leaf(conjecture));
    for (pud_node_id child : h.children_of(conjecture))
        EXPECT_TRUE(h.live_leaf(child));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, ExpandedGoalsQueryIsRemovedOnceNoLeafMentionsIt) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("color", {h.var(0)})}, 1);
    const pud_mhq_head_id expanded = h.query_of(conjecture, 0).value();
    h.expand(conjecture, 0);
    EXPECT_FALSE(h.alive(expanded));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, UnchangedGoalsQueryIsSharedByEveryChild) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(1)})}, 2);
    const pud_mhq_head_id shared = h.query_of(conjecture, 1).value();
    h.expand(conjecture, 0);
    ASSERT_EQ(h.children_of(conjecture).size(), 2u);
    for (pud_node_id child : h.children_of(conjecture))
        EXPECT_EQ(h.query_of(child, 1).value(), shared);
    EXPECT_TRUE(h.alive(shared));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, SharedQuerySurvivesUntilItsLastLeafIsRefuted) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(1)})}, 2);
    const pud_mhq_head_id shared = h.query_of(conjecture, 1).value();
    h.expand(conjecture, 0);
    const std::vector<pud_node_id> children = h.children_of(conjecture);
    ASSERT_EQ(children.size(), 2u);
    h.refute(children.at(0));
    EXPECT_TRUE(h.alive(shared));
    h.refute(children.at(1));
    EXPECT_FALSE(h.alive(shared));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, SharedQuerySurvivesUntilItsLastLeafIsExpandedPastIt) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(1)})}, 2);
    const pud_mhq_head_id shared = h.query_of(conjecture, 1).value();
    h.expand(conjecture, 0);
    const std::vector<pud_node_id> children = h.children_of(conjecture);
    ASSERT_EQ(children.size(), 2u);
    h.expand(children.at(0), 1);
    EXPECT_TRUE(h.alive(shared));
    h.expand(children.at(1), 1);
    EXPECT_FALSE(h.alive(shared));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, ForkedQueryReplacesTheParentsQueryForEveryChild) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(
        h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    const pud_mhq_head_id parents_query = h.query_of(conjecture, 1).value();
    h.expand(conjecture, 0);
    EXPECT_FALSE(h.alive(parents_query));
    for (pud_node_id child : h.children_of(conjecture)) {
        if (h.refuted(child))
            continue;
        const pud_mhq_head_id forked = h.query_of(child, 1).value();
        EXPECT_NE(forked, parents_query);
        EXPECT_TRUE(h.alive(forked));
    }
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, RefutedChildsForkedQueryIsRemoved) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(
        h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    h.expand(conjecture, 0);
    const std::vector<pud_node_id> children = h.children_of(conjecture);
    ASSERT_EQ(children.size(), 2u);
    pud_node_id live_child = children.at(0);
    for (pud_node_id child : children) {
        if (!h.refuted(child))
            live_child = child;
    }
    const pud_mhq_head_id forked = h.query_of(live_child, 1).value();
    h.refute(live_child);
    EXPECT_FALSE(h.alive(forked));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, RefutingTheRootRemovesEveryQuery) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(1)})}, 2);
    const pud_mhq_head_id colors = h.query_of(conjecture, 0).value();
    const pud_mhq_head_id warmth = h.query_of(conjecture, 1).value();
    h.refute(conjecture);
    EXPECT_FALSE(h.alive(colors));
    EXPECT_FALSE(h.alive(warmth));
}

TEST_F(PudManifestConjectureQueryLifetimeIntegrationTest, NewGoalsQueryLivesWithTheChildThatAddedIt) {
    h.add_nat();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("nat", {h.var(0)})}, 1);
    h.expand(conjecture, 0);
    pud_node_id recursive = 0;
    for (pud_node_id child : h.children_of(conjecture)) {
        if (!h.open_goals(child).empty())
            recursive = child;
    }
    ASSERT_NE(recursive, 0u);
    const pud_mhq_head_id added = h.query_of(recursive, 1).value();
    EXPECT_THAT(h.holders_of(added), ElementsAre(recursive));
    EXPECT_TRUE(h.alive(added));
    h.refute(recursive);
    EXPECT_FALSE(h.alive(added));
}

#endif
