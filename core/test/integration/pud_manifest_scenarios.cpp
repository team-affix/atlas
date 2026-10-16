#include <algorithm>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_manifest_harness.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct PudManifestScenarioTest : public ::testing::Test {
    pud_manifest_harness h;

    const expr* q(std::vector<const expr*> args) { return h.functor("q", std::move(args)); }

    std::vector<const expr*> distinct(std::vector<const expr*> answers) {
        std::sort(answers.begin(), answers.end());
        answers.erase(std::unique(answers.begin(), answers.end()), answers.end());
        return answers;
    }

    bool solvable(pud_manifest_harness& harness, const expr* goal, size_t budget) {
        const pud_node_id query = harness.start_query(harness.atom("q"), {goal}, 0);
        return harness.manifest.solver_.solve(query, budget).has_value();
    }

    std::vector<std::vector<pud_node_id>> candidate_paths(pud_node_id leaf, body_goal_id goal) {
        std::vector<std::vector<pud_node_id>> paths;
        for (pud_mhcs_head_id candidate : h.manifest.mhq_.get_candidates(h.query_of(leaf, goal).value()))
            paths.push_back(h.manifest.mhcs_.get_path(candidate));
        std::sort(paths.begin(), paths.end());
        return paths;
    }

    // p(X) :- r(X). r(a). The p tree is expanded once, so the only candidate for p(a) is its
    // child; returns that child.
    pud_node_id add_lemma_through_p() {
        const pud_node_id p_rule = h.axiom(h.functor("p", {h.var(0)}), {h.functor("r", {h.var(0)})}, 1);
        h.axiom(h.functor("r", {h.atom("a")}), {}, 0);
        h.start_tree(p_rule);
        h.manifest.leaf_expander_.expand(p_rule, 0);
        EXPECT_EQ(h.children_of(p_rule).size(), 1u);
        return h.children_of(p_rule).front();
    }
};

// ---------- sharing and renames ----------

TEST_F(PudManifestScenarioTest, AcceptingASharedQueryAtADeepDescendantSucceeds) {
    h.add_colors();
    h.axiom(h.atom("a1"), {}, 0);
    h.axiom(h.atom("a2"), {}, 0);
    h.axiom(h.atom("a3"), {}, 0);
    const pud_node_id query = h.start_query(
        q({h.var(0)}), {h.atom("a1"), h.atom("a2"), h.atom("a3"), h.functor("color", {h.var(0)})}, 1);
    const pud_mhq_head_id shared = h.query_of(query, 3).value();
    pud_node_id frontier = query;
    for (body_goal_id goal = 0; goal < 3; ++goal) {
        h.manifest.leaf_expander_.expand(frontier, goal);
        ASSERT_EQ(h.children_of(frontier).size(), 1u);
        frontier = h.children_of(frontier).front();
        EXPECT_EQ(h.query_of(frontier, 3), shared);
    }
    EXPECT_EQ(h.manifest.query_head_nodes_.get(shared), query);
    EXPECT_NO_THROW(h.manifest.leaf_expander_.expand(frontier, 3));
    EXPECT_EQ(h.children_of(frontier).size(), 2u);
    h.check_all();
    EXPECT_THAT(h.enumerate(query, 10, 50), UnorderedElementsAre(q({h.atom("red")}), q({h.atom("blue")})));
}

TEST_F(PudManifestScenarioTest, CrossGoalVariableMergeIsHandledAtAcceptance) {
    h.add_colors();
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    const pud_node_id query = h.start_query(
        q({h.var(0), h.var(1)}),
        {h.functor("eq", {h.var(0), h.var(1)}), h.functor("color", {h.var(0)}), h.functor("color", {h.var(1)})},
        2);
    std::vector<const expr*> answers;
    while (h.step(query, answers))
        h.check_all();
    EXPECT_THAT(
        answers,
        UnorderedElementsAre(q({h.atom("red"), h.atom("red")}), q({h.atom("blue"), h.atom("blue")})));
}

TEST_F(PudManifestScenarioTest, RenameOnlyStepsDoNotForkButStayCorrect) {
    h.add_colors();
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    const pud_node_id query = h.start_query(
        q({h.var(0), h.var(1)}),
        {h.functor("eq", {h.var(0), h.var(1)}), h.functor("color", {h.var(0)}), h.functor("color", {h.var(1)})},
        2);
    const pud_mhq_head_id first_color = h.query_of(query, 1).value();
    const pud_mhq_head_id second_color = h.query_of(query, 2).value();
    h.manifest.leaf_expander_.expand(query, 0);
    ASSERT_EQ(h.children_of(query).size(), 1u);
    const pud_node_id merged = h.children_of(query).front();
    EXPECT_EQ(h.query_of(merged, 1), first_color);
    EXPECT_EQ(h.query_of(merged, 2), second_color);
    EXPECT_THAT(
        h.manifest.node_added_substitutions_.get(merged),
        ElementsAre(pud_substitution{.var_idx = 1, .value = h.var(0)}));
    h.check_all();
    EXPECT_THAT(
        h.enumerate(query, 10, 50),
        UnorderedElementsAre(q({h.atom("red"), h.atom("red")}), q({h.atom("blue"), h.atom("blue")})));
}

TEST_F(PudManifestScenarioTest, ForkedQueriesOnlyListCandidatesThatStillUnify) {
    h.add_colors();
    const pud_node_id query = h.start_query(
        q({h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    h.manifest.leaf_expander_.expand(query, 0);
    size_t live_children = 0;
    for (pud_node_id child : h.children_of(query)) {
        if (h.refuted(child))
            continue;
        ++live_children;
        for (const std::vector<pud_node_id>& path : candidate_paths(child, 1))
            EXPECT_EQ(h.manifest.node_heads_.get(path.front()), h.functor("warm", {h.atom("red")}));
    }
    EXPECT_EQ(live_children, 1u);
    h.check_every_candidate_is_acceptable_at_every_leaf_holding_the_query();
}

// ---------- self-reference and lemmas ----------

TEST_F(PudManifestScenarioTest, QueryTreeIsItsOwnCandidate) {
    const pud_node_id fact = h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
    const pud_node_id query = h.start_query(
        h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
    EXPECT_THAT(
        candidate_paths(query, 0),
        UnorderedElementsAre(std::vector<pud_node_id>{fact}, std::vector<pud_node_id>{query}));
    const std::optional<pud_node_id> solution = h.manifest.solver_.solve(query, 10);
    ASSERT_TRUE(solution.has_value());
    EXPECT_EQ(h.answer(query, solution.value()), h.functor("nat", {h.peano(1)}));
    h.check_all();
}

TEST_F(PudManifestScenarioTest, ExpandingTheLeafThatIsItsOwnGoalsCandidate) {
    h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
    const pud_node_id query = h.start_query(
        h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
    EXPECT_NO_THROW(h.manifest.leaf_expander_.expand(query, 0));
    EXPECT_EQ(h.children_of(query).size(), 2u);
    h.check_all();
    for (uint32_t count = 1; count <= 6; ++count)
        EXPECT_TRUE(solvable(h, h.functor("nat", {h.peano(count)}), 100)) << count;
    EXPECT_FALSE(solvable(h, h.functor("nat", {h.peano_on(3, "bad")}), 100));
    h.check_all();
}

TEST_F(PudManifestScenarioTest, OneQueryUsesAnotherQueryAsALemma) {
    h.add_colors();
    const pud_node_id first = h.start_query(
        h.functor("warmcolor", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    const pud_node_id second = h.start_query(q({h.var(0)}), {h.functor("warmcolor", {h.var(0)})}, 1);
    bool uses_first = false;
    for (const std::vector<pud_node_id>& path : candidate_paths(second, 0))
        uses_first = uses_first || path.front() == first;
    EXPECT_TRUE(uses_first);
    EXPECT_THAT(distinct(h.enumerate(second, 10, 50)), ElementsAre(q({h.atom("red")})));
    h.check_all();
}

TEST_F(PudManifestScenarioTest, CandidatesMoveDeeperWhenAnAxiomTreeGrows) {
    const pud_node_id rule = h.add_nat();
    h.start_tree(rule);
    h.grow_along_open_goals(rule, 3);
    const pud_node_id query = h.start_query(h.atom("q"), {h.functor("nat", {h.peano(3)})}, 0);
    size_t deepest = 0;
    for (const std::vector<pud_node_id>& path : candidate_paths(query, 0))
        deepest = std::max(deepest, path.size());
    EXPECT_GE(deepest, 3u);
    h.check_all();
    EXPECT_TRUE(h.manifest.solver_.solve(query, 3).has_value());
}

TEST_F(PudManifestScenarioTest, LemmaPathReplayMatchesFlatResolution) {
    pud_manifest_harness flat;
    flat.add_nat();
    const pud_node_id rule = h.add_nat();
    h.start_tree(rule);
    h.grow_along_open_goals(rule, 4);
    for (uint32_t count = 0; count <= 8; ++count) {
        EXPECT_TRUE(solvable(flat, flat.functor("nat", {flat.peano(count)}), 200)) << count;
        EXPECT_TRUE(solvable(h, h.functor("nat", {h.peano(count)}), 200)) << count;
    }
    EXPECT_FALSE(solvable(flat, flat.functor("nat", {flat.peano_on(2, "bad")}), 200));
    EXPECT_FALSE(solvable(h, h.functor("nat", {h.peano_on(2, "bad")}), 200));
    const pud_node_id enumerating = h.start_query(q({h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    const std::vector<const expr*> answers = distinct(h.enumerate(enumerating, 20, 300));
    for (uint32_t count = 0; count < 4; ++count)
        EXPECT_NE(std::find(answers.begin(), answers.end(), q({h.peano(count)})), answers.end()) << count;
    h.check_all();
}

// ---------- refutation cascades ----------

TEST_F(PudManifestScenarioTest, RefutingALemmaSubtreeRefutesQueriesThatOnlyHadIt) {
    const pud_node_id lemma = add_lemma_through_p();
    const pud_node_id query = h.start_query(h.atom("q"), {h.functor("p", {h.atom("a")})}, 0);
    ASSERT_THAT(candidate_paths(query, 0), ElementsAre(h.path_to(lemma)));
    h.manifest.node_refuter_.refute(lemma);
    EXPECT_TRUE(h.refuted(query));
    EXPECT_FALSE(h.manifest.solver_.solve(query, 50).has_value());
    h.check_all();
}

TEST_F(PudManifestScenarioTest, RefutingOneOfTwoCandidatesMakesTheQueryUnit) {
    const pud_node_id red = h.axiom(h.functor("color", {h.atom("red")}), {}, 0);
    h.axiom(h.functor("color", {h.atom("blue")}), {}, 0);
    const pud_node_id query = h.start_query(q({h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    ASSERT_EQ(candidate_paths(query, 0).size(), 2u);
    h.manifest.node_refuter_.refute(red);
    EXPECT_EQ(candidate_paths(query, 0).size(), 1u);
    EXPECT_FALSE(h.refuted(query));
    EXPECT_THAT(h.enumerate(query, 10, 50), ElementsAre(q({h.atom("blue")})));
    h.check_all();
}

TEST_F(PudManifestScenarioTest, AllChildrenRefutedOnArrivalRefuteTheParent) {
    h.add_colors();
    h.axiom(h.functor("cold", {h.atom("green")}), {}, 0);
    const pud_node_id query = h.start_query(
        h.atom("q"), {h.functor("color", {h.var(0)}), h.functor("cold", {h.var(0)})}, 1);
    ASSERT_FALSE(h.refuted(query));
    h.manifest.leaf_expander_.expand(query, 0);
    EXPECT_EQ(h.children_of(query).size(), 2u);
    for (pud_node_id child : h.children_of(query))
        EXPECT_TRUE(h.refuted(child));
    EXPECT_TRUE(h.refuted(query));
    h.check_all();
}

TEST_F(PudManifestScenarioTest, CascadeCrossesTrees) {
    const pud_node_id lemma = add_lemma_through_p();
    const pud_node_id first = h.start_query(h.atom("q1"), {h.functor("p", {h.atom("a")})}, 0);
    const pud_node_id second = h.start_query(h.atom("q2"), {h.functor("p", {h.atom("a")})}, 0);
    const pud_node_id independent = h.start_query(h.atom("q3"), {h.functor("r", {h.atom("a")})}, 0);
    h.manifest.node_refuter_.refute(lemma);
    EXPECT_TRUE(h.refuted(first));
    EXPECT_TRUE(h.refuted(second));
    EXPECT_FALSE(h.refuted(independent));
    EXPECT_TRUE(h.manifest.solver_.solve(independent, 10).has_value());
    h.check_all();
}

TEST_F(PudManifestScenarioTest, RefutingAnAlreadyRefutedNodeIsANoop) {
    h.add_colors();
    const pud_node_id query = h.start_query(q({h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    h.manifest.leaf_expander_.expand(query, 0);
    const pud_node_id child = h.children_of(query).front();
    h.manifest.node_refuter_.refute(child);
    EXPECT_NO_THROW(h.manifest.node_refuter_.refute(child));
    EXPECT_NO_THROW(h.manifest.node_refuter_.refute(child));
    EXPECT_FALSE(h.refuted(query));
    h.check_all();
}

TEST_F(PudManifestScenarioTest, RefutedRootStaysUnsolvable) {
    h.add_colors();
    const pud_node_id query = h.start_query(q({h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    h.manifest.node_refuter_.refute(query);
    for (int attempt = 0; attempt < 3; ++attempt)
        EXPECT_FALSE(h.manifest.solver_.solve(query, 50).has_value());
    EXPECT_NO_THROW(h.manifest.node_refuter_.refute(query));
    h.check_all();
}

// ---------- unification edge cases ----------

TEST_F(PudManifestScenarioTest, OccursCheckRefutesCyclicGoals) {
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    const pud_node_id query = h.start_query(h.atom("q"), {h.functor("eq", {h.var(0), h.functor("f", {h.var(0)})})}, 1);
    EXPECT_TRUE(h.refuted(query));
    EXPECT_FALSE(h.manifest.solver_.solve(query, 10).has_value());
    h.check_all();
}

TEST_F(PudManifestScenarioTest, RepeatedVariablesInHeadsAreRespected) {
    h.axiom(h.functor("same", {h.var(0), h.var(0)}), {}, 1);
    const pud_node_id different = h.start_query(h.atom("q1"), {h.functor("same", {h.atom("a"), h.atom("b")})}, 0);
    const pud_node_id equal = h.start_query(h.atom("q2"), {h.functor("same", {h.atom("a"), h.atom("a")})}, 0);
    const pud_node_id one_side = h.start_query(
        h.functor("q3", {h.var(0)}), {h.functor("same", {h.var(0), h.atom("c")})}, 1);
    const pud_node_id both_free = h.start_query(
        h.functor("q4", {h.var(0), h.var(1)}), {h.functor("same", {h.var(0), h.var(1)})}, 2);
    EXPECT_TRUE(h.refuted(different));
    EXPECT_THAT(h.enumerate(equal, 5, 20), ElementsAre(h.atom("q2")));
    EXPECT_THAT(h.enumerate(one_side, 5, 20), ElementsAre(h.functor("q3", {h.atom("c")})));
    EXPECT_THAT(h.enumerate(both_free, 5, 20), ElementsAre(h.functor("q4", {h.var(0), h.var(0)})));
    h.check_all();
}

TEST_F(PudManifestScenarioTest, GroundGoalsNeedNoSubstitutions) {
    h.add_colors();
    const pud_node_id query = h.start_query(h.atom("q"), {h.functor("color", {h.atom("red")})}, 0);
    h.manifest.leaf_expander_.expand(query, 0);
    ASSERT_EQ(h.children_of(query).size(), 1u);
    const pud_node_id child = h.children_of(query).front();
    EXPECT_THAT(h.manifest.node_added_substitutions_.get(child), IsEmpty());
    EXPECT_EQ(h.manifest.node_added_var_count_.get(child), 0u);
    EXPECT_THAT(h.manifest.node_added_body_goals_.get(child), IsEmpty());
    h.check_all();
}

TEST_F(PudManifestScenarioTest, AnswerKeepsUnboundVariables) {
    h.add_append();
    const pud_node_id query = h.start_query(q({h.var(0)}), {h.functor("app", {h.atom("nil"), h.var(0), h.var(0)})}, 1);
    const std::optional<pud_node_id> solution = h.manifest.solver_.solve(query, 10);
    ASSERT_TRUE(solution.has_value());
    EXPECT_EQ(h.answer(query, solution.value()), q({h.var(0)}));
    h.check_all();
}

TEST_F(PudManifestScenarioTest, LargeFrameOffsetsDoNotCollide) {
    std::vector<const expr*> tuple_args;
    for (uint32_t index = 0; index < 10; ++index)
        tuple_args.push_back(h.var(index));
    tuple_args.push_back(h.functor("f", {h.var(0), h.var(9)}));
    h.axiom(h.functor("tup", tuple_args), {}, 10);
    std::vector<const expr*> query_args;
    for (uint32_t index = 0; index < 20; ++index)
        query_args.push_back(h.var(index));
    std::vector<const expr*> goal_args;
    for (uint32_t index = 5; index < 15; ++index)
        goal_args.push_back(h.var(index));
    goal_args.push_back(h.var(19));
    const pud_node_id query = h.start_query(h.functor("q", query_args), {h.functor("tup", goal_args)}, 20);
    const std::optional<pud_node_id> solution = h.manifest.solver_.solve(query, 10);
    ASSERT_TRUE(solution.has_value());
    std::vector<const expr*> expected_args;
    for (uint32_t index = 0; index < 19; ++index)
        expected_args.push_back(h.var(index));
    expected_args.push_back(h.functor("f", {h.var(5), h.var(14)}));
    EXPECT_EQ(h.answer(query, solution.value()), h.functor("q", expected_args));
    h.check_all();
}

// ---------- scale ----------

TEST_F(PudManifestScenarioTest, LongAppendIsCorrect) {
    h.add_append();
    std::vector<const expr*> front;
    std::vector<const expr*> back;
    for (int index = 0; index < 50; ++index) {
        front.push_back(h.atom("a" + std::to_string(index)));
        back.push_back(h.atom("b" + std::to_string(index)));
    }
    std::vector<const expr*> both = front;
    both.insert(both.end(), back.begin(), back.end());
    const pud_node_id query = h.start_query(
        q({h.var(0)}), {h.functor("app", {h.list(front), h.list(back), h.var(0)})}, 1);
    EXPECT_THAT(h.enumerate(query, 5, 200), ElementsAre(q({h.list(both)})));
    EXPECT_TRUE(h.refuted(query));
    h.check_all();
}

TEST_F(PudManifestScenarioTest, ManyQueriesInterleaved) {
    h.add_colors();
    h.add_nat();
    h.add_member();
    const pud_node_id colors = h.start_query(h.functor("c", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    const pud_node_id numbers = h.start_query(h.functor("n", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    const pud_node_id members = h.start_query(
        h.functor("m", {h.var(0)}), {h.functor("mem", {h.var(0), h.list({h.atom("a"), h.atom("b")})})}, 1);
    std::vector<const expr*> color_answers;
    std::vector<const expr*> number_answers;
    std::vector<const expr*> member_answers;
    for (int round = 0; round < 40; ++round) {
        h.step(colors, color_answers);
        h.step(numbers, number_answers);
        h.step(members, member_answers);
        if (round % 8 == 0)
            h.check_all();
    }
    EXPECT_THAT(color_answers, UnorderedElementsAre(h.functor("c", {h.atom("red")}), h.functor("c", {h.atom("blue")})));
    EXPECT_THAT(member_answers, ElementsAre(h.functor("m", {h.atom("a")}), h.functor("m", {h.atom("b")})));
    ASSERT_GE(number_answers.size(), 5u);
    for (size_t index = 0; index < number_answers.size(); ++index)
        EXPECT_EQ(number_answers.at(index), h.functor("n", {h.peano(static_cast<uint32_t>(index))}));
    h.check_all();
}

// The normalizers, the unifier and its occurs check recurse once per term level, so a term this
// deep overflows the stack and takes the whole test binary down. Enable to reproduce.
TEST_F(PudManifestScenarioTest, DISABLED_VeryDeepTermsDoNotOverflowTheStack) {
    h.add_nat();
    const pud_node_id query = h.start_query(h.atom("q"), {h.functor("nat", {h.peano(300000)})}, 0);
    EXPECT_FALSE(h.refuted(query));
}

// ---------- lifetime and destruction ----------

TEST_F(PudManifestScenarioTest, ManifestDestroyedMidSolveIsClean) {
    for (size_t round = 0; round < 25; ++round) {
        auto harness = std::make_unique<pud_manifest_harness>();
        harness->add_nat();
        harness->add_append();
        const pud_node_id numbers = harness->start_query(
            harness->functor("n", {harness->var(0)}), {harness->functor("nat", {harness->var(0)})}, 1);
        const pud_node_id splits = harness->start_query(
            harness->functor("s", {harness->var(0), harness->var(1)}),
            {harness->functor("app", {harness->var(0), harness->var(1), harness->list({harness->atom("x"), harness->atom("y")})})},
            2);
        harness->manifest.solver_.solve(numbers, round);
        harness->manifest.solver_.solve(splits, round / 2);
        harness->check_all();
        EXPECT_NO_THROW(harness.reset());
    }
}

TEST_F(PudManifestScenarioTest, AllQueriesGoneAfterRefutingEveryRoot) {
    h.add_colors();
    h.add_nat();
    h.add_append();
    const pud_node_id conjunction = h.start_query(
        q({h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    const pud_node_id numbers = h.start_query(h.functor("n", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    const pud_node_id splits = h.start_query(
        h.functor("s", {h.var(0), h.var(1)}),
        {h.functor("app", {h.var(0), h.var(1), h.list({h.atom("x"), h.atom("y")})})},
        2);
    std::set<pud_mhq_head_id> seen;
    std::vector<const expr*> answers;
    for (int round = 0; round < 15; ++round) {
        for (pud_node_id query : {conjunction, numbers, splits}) {
            h.step(query, answers);
            for (pud_mhq_head_id head : h.heads_held_by_leaves())
                seen.insert(head);
        }
    }
    ASSERT_FALSE(seen.empty());
    for (pud_node_id root : h.started_roots)
        h.manifest.node_refuter_.refute(root);
    for (pud_mhq_head_id head : seen)
        EXPECT_FALSE(h.alive(head)) << "head " << head;
    EXPECT_TRUE(h.live_leaves_with_descents().empty());
    h.check_all();
}

TEST_F(PudManifestScenarioTest, AxiomTreeQueriesLiveOnlyWhileItsLeavesDo) {
    const pud_node_id rule = h.add_nat();
    h.start_tree(rule);
    h.grow_along_open_goals(rule, 3);
    const std::vector<pud_mhq_head_id> heads = h.heads_held_by_leaves();
    ASSERT_FALSE(heads.empty());
    for (pud_mhq_head_id head : heads)
        EXPECT_TRUE(h.alive(head));
    h.manifest.node_refuter_.refute(rule);
    for (pud_mhq_head_id head : heads)
        EXPECT_FALSE(h.alive(head)) << "head " << head;
    h.check_all();
}

// ---------- forced queries and budget ----------

TEST_F(PudManifestScenarioTest, SingleCandidateGoalIsReportedUnit) {
    h.add_colors();
    const pud_node_id unit = h.start_query(h.atom("q1"), {h.functor("warm", {h.var(0)})}, 1);
    const pud_node_id live = h.start_query(h.atom("q2"), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_THAT(h.manifest.node_descents_.get(unit).body_goal_pq().unit_goals(), ElementsAre(0u));
    EXPECT_THAT(h.manifest.node_descents_.get(live).body_goal_pq().unit_goals(), IsEmpty());
    EXPECT_EQ(candidate_paths(unit, 0).size(), 1u);
}

TEST_F(PudManifestScenarioTest, BudgetedSolveResumesWhereItStopped) {
    pud_manifest_harness whole;
    pud_manifest_harness stepped;
    whole.add_nat();
    stepped.add_nat();
    const pud_node_id whole_query = whole.start_query(whole.atom("q"), {whole.functor("nat", {whole.peano(6)})}, 0);
    const pud_node_id stepped_query = stepped.start_query(stepped.atom("q"), {stepped.functor("nat", {stepped.peano(6)})}, 0);
    const std::optional<pud_node_id> at_once = whole.manifest.solver_.solve(whole_query, 100);
    std::optional<pud_node_id> one_by_one;
    for (int attempt = 0; attempt < 100 && !one_by_one.has_value(); ++attempt)
        one_by_one = stepped.manifest.solver_.solve(stepped_query, 1);
    ASSERT_TRUE(at_once.has_value());
    ASSERT_TRUE(one_by_one.has_value());
    EXPECT_EQ(at_once, one_by_one);
    EXPECT_EQ(whole.manifest.node_id_sequencer_.peek(), stepped.manifest.node_id_sequencer_.peek());
}
