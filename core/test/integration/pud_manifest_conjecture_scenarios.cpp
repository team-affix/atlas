#include <algorithm>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_manifest_harness.hpp"

using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::UnorderedElementsAre;

// h solves conjectures with the solver; d drives the database by hand, for tests that expand or
// refute conjecture leaves themselves.
struct PudManifestConjectureScenarioIntegrationTest : public ::testing::Test {
    pud_manifest_harness h{0, pud_harness_scope::conjecture_leaves};
    pud_manifest_harness d{0, pud_harness_scope::every_open_leaf};

    const expr* q(pud_manifest_harness& harness, std::vector<const expr*> args) {
        return harness.functor("q", std::move(args));
    }

    std::vector<const expr*> distinct(std::vector<const expr*> answers) {
        std::sort(answers.begin(), answers.end());
        answers.erase(std::unique(answers.begin(), answers.end()), answers.end());
        return answers;
    }

    bool solvable(pud_manifest_harness& harness, const expr* goal, size_t budget) {
        const pud_node_id conjecture = harness.start_conjecture(harness.atom("q"), {goal}, 0);
        return harness.next_solution(conjecture, budget).has_value();
    }

    std::vector<std::vector<pud_node_id>> candidate_paths(pud_manifest_harness& harness, pud_node_id leaf, pud_body_goal_id goal) {
        std::vector<std::vector<pud_node_id>> paths;
        for (pud_mhcs_head_id candidate : harness.manifest.mhq_.candidates(harness.query_of(leaf, goal).value()))
            paths.push_back(harness.manifest.mhcs_.path(candidate));
        std::sort(paths.begin(), paths.end());
        return paths;
    }

    pud_node_id add_lemma_through_p(pud_manifest_harness& harness) {
        const pud_node_id p_rule = harness.axiom(harness.functor("p", {harness.var(0)}), {harness.functor("r", {harness.var(0)})}, 1);
        harness.axiom(harness.functor("r", {harness.atom("a")}), {}, 0);
        harness.open_axioms();
        harness.expand(p_rule, 0);
        EXPECT_EQ(harness.children_of(p_rule).size(), 1u);
        return harness.children_of(p_rule).front();
    }
};

TEST_F(PudManifestConjectureScenarioIntegrationTest, AcceptingASharedQueryAtADeepDescendantSucceeds) {
    d.add_colors();
    d.axiom(d.atom("a1"), {}, 0);
    d.axiom(d.atom("a2"), {}, 0);
    d.axiom(d.atom("a3"), {}, 0);
    const pud_node_id conjecture = d.start_conjecture(
        q(d, {d.var(0)}), {d.atom("a1"), d.atom("a2"), d.atom("a3"), d.functor("color", {d.var(0)})}, 1);
    const pud_mhq_head_id shared = d.query_of(conjecture, 3).value();
    pud_node_id frontier = conjecture;
    for (pud_body_goal_id goal = 0; goal < 3; ++goal) {
        d.expand(frontier, goal);
        ASSERT_EQ(d.children_of(frontier).size(), 1u);
        frontier = d.children_of(frontier).front();
        EXPECT_EQ(d.query_of(frontier, 3), shared);
    }
    EXPECT_THAT(d.holders_of(shared), ElementsAre(frontier));
    EXPECT_NO_THROW(d.expand(frontier, 3));
    EXPECT_EQ(d.children_of(frontier).size(), 2u);
    d.check_all();
    EXPECT_THAT(d.proven_answers(conjecture, 50), UnorderedElementsAre(q(d, {d.atom("red")}), q(d, {d.atom("blue")})));
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, CrossGoalVariableMergeIsHandledAtAcceptance) {
    h.add_colors();
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    const pud_node_id conjecture = h.start_conjecture(
        q(h, {h.var(0), h.var(1)}),
        {h.functor("eq", {h.var(0), h.var(1)}), h.functor("color", {h.var(0)}), h.functor("color", {h.var(1)})},
        2);
    std::vector<const expr*> answers;
    while (h.step(conjecture, answers))
        h.check_all();
    EXPECT_THAT(
        answers,
        UnorderedElementsAre(q(h, {h.atom("red"), h.atom("red")}), q(h, {h.atom("blue"), h.atom("blue")})));
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, RenameOnlyStepsDoNotForkButStayCorrect) {
    d.add_colors();
    d.axiom(d.functor("eq", {d.var(0), d.var(0)}), {}, 1);
    const pud_node_id conjecture = d.start_conjecture(
        q(d, {d.var(0), d.var(1)}),
        {d.functor("eq", {d.var(0), d.var(1)}), d.functor("color", {d.var(0)}), d.functor("color", {d.var(1)})},
        2);
    const pud_mhq_head_id first_color = d.query_of(conjecture, 1).value();
    const pud_mhq_head_id second_color = d.query_of(conjecture, 2).value();
    d.expand(conjecture, 0);
    ASSERT_EQ(d.children_of(conjecture).size(), 1u);
    const pud_node_id merged = d.children_of(conjecture).front();
    EXPECT_EQ(d.query_of(merged, 1), first_color);
    EXPECT_EQ(d.query_of(merged, 2), second_color);
    EXPECT_THAT(
        d.manifest.node_substitutions_.get(merged),
        ElementsAre(pud_substitution{.var_idx = 1, .value = d.var(0)}));
    d.check_all();
    EXPECT_THAT(
        d.proven_answers(conjecture, 50),
        UnorderedElementsAre(q(d, {d.atom("red"), d.atom("red")}), q(d, {d.atom("blue"), d.atom("blue")})));
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, ForkedQueriesOnlyListCandidatesThatStillUnify) {
    d.add_colors();
    const pud_node_id conjecture = d.start_conjecture(
        q(d, {d.var(0)}), {d.functor("color", {d.var(0)}), d.functor("warm", {d.var(0)})}, 1);
    d.expand(conjecture, 0);
    size_t live_children = 0;
    for (pud_node_id child : d.children_of(conjecture)) {
        if (d.refuted(child))
            continue;
        ++live_children;
        for (const std::vector<pud_node_id>& path : candidate_paths(d, child, 1))
            EXPECT_EQ(d.manifest.root_heads_.get(path.front()), d.functor("warm", {d.atom("red")}));
    }
    EXPECT_EQ(live_children, 1u);
    d.check_every_candidate_is_acceptable_at_every_leaf_holding_the_query();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, AxiomTreeIsItsOwnCandidate) {
    const pud_node_id fact = h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
    const pud_node_id rule = h.axiom(
        h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
    h.open_axioms();
    EXPECT_THAT(
        candidate_paths(h, rule, 0),
        UnorderedElementsAre(std::vector<pud_node_id>{fact}, std::vector<pud_node_id>{rule}));
    const pud_node_id conjecture = h.start_conjecture(q(h, {h.var(0)}), {h.functor("nat", {h.functor("s", {h.var(0)})})}, 1);
    const std::optional<pud_solution> solution = h.next_solution(conjecture, 10);
    ASSERT_TRUE(solution.has_value());
    EXPECT_EQ(solution->answer, q(h, {h.atom("z")}));
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, ExpandingTheLeafThatIsItsOwnGoalsCandidate) {
    h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
    const pud_node_id rule = h.axiom(
        h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
    h.open_axioms();
    EXPECT_NO_THROW(h.expand(rule, 0));
    EXPECT_EQ(h.children_of(rule).size(), 2u);
    h.check_all();
    for (uint32_t count = 1; count <= 6; ++count)
        EXPECT_TRUE(solvable(h, h.functor("nat", {h.peano(count)}), 100)) << count;
    EXPECT_FALSE(solvable(h, h.functor("nat", {h.peano_on(3, "bad")}), 100));
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, OneQueryUsesAnAxiomRuleAsALemma) {
    h.add_colors();
    const pud_node_id warmcolor = h.axiom(
        h.functor("warmcolor", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    h.open_axioms();
    const pud_node_id conjecture = h.start_conjecture(q(h, {h.var(0)}), {h.functor("warmcolor", {h.var(0)})}, 1);
    bool uses_rule = false;
    for (const std::vector<pud_node_id>& path : candidate_paths(h, conjecture, 0))
        uses_rule = uses_rule || path.front() == warmcolor;
    EXPECT_TRUE(uses_rule);
    EXPECT_THAT(distinct(h.enumerate(conjecture, 10, 50)), ElementsAre(q(h, {h.atom("red")})));
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, ConjectureIsNeverACandidateForAnotherConjecture) {
    h.add_colors();
    h.start_conjecture(
        h.functor("warmcolor", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    const pud_node_id second = h.start_conjecture(q(h, {h.var(0)}), {h.functor("warmcolor", {h.var(0)})}, 1);
    EXPECT_TRUE(h.refuted(second));
    EXPECT_THAT(h.enumerate(second, 10, 50), IsEmpty());
    EXPECT_TRUE(h.exhausted(second));
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, ConjectureIsNeverACandidateForItsOwnGoals) {
    const pud_node_id fact = h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
    const pud_node_id conjecture = h.start_conjecture(
        h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
    EXPECT_THAT(candidate_paths(h, conjecture, 0), ElementsAre(std::vector<pud_node_id>{fact}));
    EXPECT_THAT(distinct(h.enumerate(conjecture, 10, 50)),
        ElementsAre(h.functor("nat", {h.functor("s", {h.atom("z")})})));
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, ConjectureNeverAddsAnswersToTheDatabase) {
    h.add_colors();
    h.start_conjecture(h.functor("color", {h.atom("green")}), {}, 0);
    h.start_conjecture(h.functor("color", {h.var(0)}), {h.functor("warm", {h.var(0)})}, 1);
    const pud_node_id colors = h.start_conjecture(q(h, {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_THAT(distinct(h.enumerate(colors, 10, 50)),
        UnorderedElementsAre(q(h, {h.atom("red")}), q(h, {h.atom("blue")})));
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, ConjectureIsARootLeafButNotAnAxiom) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(q(h, {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_TRUE(h.root(conjecture));
    EXPECT_THAT(h.manifest.axioms_, ::testing::Not(::testing::Contains(conjecture)));
    EXPECT_EQ(h.manifest.root_heads_.get(conjecture), q(h, {h.var(0)})) << "its head is only the answer template";
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, ConcurrentConjecturesNeverSeeEachOther) {
    h.add_colors();
    const pud_node_id first = h.start_conjecture(q(h, {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    const pud_node_id second = h.start_conjecture(q(h, {h.var(0)}), {h.functor("warm", {h.var(0)})}, 1);
    std::vector<const expr*> first_answers;
    std::vector<const expr*> second_answers;
    bool first_running = true;
    bool second_running = true;
    for (size_t round = 0; round < 50 && (first_running || second_running); ++round) {
        if (first_running)
            first_running = h.step(first, first_answers);
        if (second_running)
            second_running = h.step(second, second_answers);
    }
    EXPECT_THAT(distinct(first_answers), UnorderedElementsAre(q(h, {h.atom("red")}), q(h, {h.atom("blue")})));
    EXPECT_THAT(distinct(second_answers), ElementsAre(q(h, {h.atom("red")})));
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, CandidatesMoveDeeperWhenAnAxiomTreeGrows) {
    const pud_node_id rule = h.add_nat();
    h.open_axioms();
    h.grow_along_open_goals(rule, 3);
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("nat", {h.peano(3)})}, 0);
    size_t deepest = 0;
    for (const std::vector<pud_node_id>& path : candidate_paths(h, conjecture, 0))
        deepest = std::max(deepest, path.size());
    EXPECT_GE(deepest, 3u);
    h.check_all();
    EXPECT_TRUE(h.next_solution(conjecture, 3).has_value());
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, LemmaPathReplayMatchesFlatResolution) {
    pud_manifest_harness flat{0, pud_harness_scope::conjecture_leaves};
    flat.add_nat();
    const pud_node_id rule = h.add_nat();
    h.open_axioms();
    h.grow_along_open_goals(rule, 4);
    for (uint32_t count = 0; count <= 8; ++count) {
        EXPECT_TRUE(solvable(flat, flat.functor("nat", {flat.peano(count)}), 200)) << count;
        EXPECT_TRUE(solvable(h, h.functor("nat", {h.peano(count)}), 200)) << count;
    }
    EXPECT_FALSE(solvable(flat, flat.functor("nat", {flat.peano_on(2, "bad")}), 200));
    EXPECT_FALSE(solvable(h, h.functor("nat", {h.peano_on(2, "bad")}), 200));
    const pud_node_id enumerating = h.start_conjecture(q(h, {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    const std::vector<const expr*> answers = distinct(h.enumerate(enumerating, 20, 300));
    for (uint32_t count = 0; count < 4; ++count)
        EXPECT_NE(std::find(answers.begin(), answers.end(), q(h, {h.peano(count)})), answers.end()) << count;
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, RefutingALemmaSubtreeRefutesQueriesThatOnlyHadIt) {
    const pud_node_id lemma = add_lemma_through_p(d);
    const pud_node_id conjecture = d.start_conjecture(d.atom("q"), {d.functor("p", {d.atom("a")})}, 0);
    ASSERT_THAT(candidate_paths(d, conjecture, 0), ElementsAre(d.path_to(lemma)));
    d.refute(lemma);
    EXPECT_TRUE(d.refuted(conjecture));
    EXPECT_THAT(d.proven_answers(conjecture, 50), IsEmpty());
    d.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, RefutingOneOfTwoCandidatesMakesTheQueryUnit) {
    const pud_node_id red = d.axiom(d.functor("color", {d.atom("red")}), {}, 0);
    d.axiom(d.functor("color", {d.atom("blue")}), {}, 0);
    const pud_node_id conjecture = d.start_conjecture(q(d, {d.var(0)}), {d.functor("color", {d.var(0)})}, 1);
    ASSERT_EQ(candidate_paths(d, conjecture, 0).size(), 2u);
    d.refute(red);
    EXPECT_EQ(candidate_paths(d, conjecture, 0).size(), 1u);
    EXPECT_FALSE(d.refuted(conjecture));
    EXPECT_THAT(d.proven_answers(conjecture, 50), ElementsAre(q(d, {d.atom("blue")})));
    d.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, AllChildrenRefutedOnArrivalRefuteTheParent) {
    d.add_colors();
    d.axiom(d.functor("cold", {d.atom("green")}), {}, 0);
    const pud_node_id conjecture = d.start_conjecture(
        d.atom("q"), {d.functor("color", {d.var(0)}), d.functor("cold", {d.var(0)})}, 1);
    ASSERT_FALSE(d.refuted(conjecture));
    d.expand(conjecture, 0);
    EXPECT_EQ(d.children_of(conjecture).size(), 2u);
    for (pud_node_id child : d.children_of(conjecture))
        EXPECT_TRUE(d.refuted(child));
    EXPECT_TRUE(d.refuted(conjecture));
    d.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, CascadeCrossesTrees) {
    const pud_node_id lemma = add_lemma_through_p(d);
    const pud_node_id first = d.start_conjecture(d.atom("q1"), {d.functor("p", {d.atom("a")})}, 0);
    const pud_node_id second = d.start_conjecture(d.atom("q2"), {d.functor("p", {d.atom("a")})}, 0);
    const pud_node_id independent = d.start_conjecture(d.atom("q3"), {d.functor("r", {d.atom("a")})}, 0);
    d.refute(lemma);
    EXPECT_TRUE(d.refuted(first));
    EXPECT_TRUE(d.refuted(second));
    EXPECT_FALSE(d.refuted(independent));
    EXPECT_THAT(d.proven_answers(independent, 10), Not(IsEmpty()));
    d.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, RefutingAnAlreadyRefutedNodeIsANoop) {
    d.add_colors();
    const pud_node_id conjecture = d.start_conjecture(q(d, {d.var(0)}), {d.functor("color", {d.var(0)})}, 1);
    d.expand(conjecture, 0);
    const pud_node_id child = d.children_of(conjecture).front();
    d.refute(child);
    EXPECT_NO_THROW(d.refute(child));
    EXPECT_NO_THROW(d.refute(child));
    EXPECT_FALSE(d.refuted(conjecture));
    d.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, RefutedRootStaysUnsolvable) {
    d.add_colors();
    const pud_node_id conjecture = d.start_conjecture(q(d, {d.var(0)}), {d.functor("color", {d.var(0)})}, 1);
    d.refute(conjecture);
    for (int attempt = 0; attempt < 3; ++attempt)
        EXPECT_THAT(d.proven_answers(conjecture, 50), IsEmpty());
    EXPECT_NO_THROW(d.refute(conjecture));
    d.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, OccursCheckRefutesCyclicGoals) {
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("eq", {h.var(0), h.functor("f", {h.var(0)})})}, 1);
    EXPECT_TRUE(h.refuted(conjecture));
    EXPECT_FALSE(h.next_solution(conjecture, 10).has_value());
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, RepeatedVariablesInHeadsAreRespected) {
    h.axiom(h.functor("same", {h.var(0), h.var(0)}), {}, 1);
    const pud_node_id different = h.start_conjecture(h.atom("q1"), {h.functor("same", {h.atom("a"), h.atom("b")})}, 0);
    const pud_node_id equal = h.start_conjecture(h.atom("q2"), {h.functor("same", {h.atom("a"), h.atom("a")})}, 0);
    const pud_node_id one_side = h.start_conjecture(
        h.functor("q3", {h.var(0)}), {h.functor("same", {h.var(0), h.atom("c")})}, 1);
    const pud_node_id both_free = h.start_conjecture(
        h.functor("q4", {h.var(0), h.var(1)}), {h.functor("same", {h.var(0), h.var(1)})}, 2);
    EXPECT_TRUE(h.refuted(different));
    EXPECT_THAT(h.enumerate(equal, 5, 20), ElementsAre(h.atom("q2")));
    EXPECT_THAT(h.enumerate(one_side, 5, 20), ElementsAre(h.functor("q3", {h.atom("c")})));
    EXPECT_THAT(h.enumerate(both_free, 5, 20), ElementsAre(h.functor("q4", {h.var(0), h.var(0)})));
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, GroundGoalsNeedNoSubstitutions) {
    d.add_colors();
    const pud_node_id conjecture = d.start_conjecture(d.atom("q"), {d.functor("color", {d.atom("red")})}, 0);
    d.expand(conjecture, 0);
    ASSERT_EQ(d.children_of(conjecture).size(), 1u);
    const pud_node_id child = d.children_of(conjecture).front();
    EXPECT_THAT(d.manifest.node_substitutions_.get(child), IsEmpty());
    EXPECT_EQ(d.manifest.node_lvcs_.get(child), d.manifest.node_lvcs_.get(conjecture));
    EXPECT_THAT(d.open_goals(child), IsEmpty());
    d.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, AnswerKeepsUnboundVariables) {
    h.add_append();
    const pud_node_id conjecture = h.start_conjecture(q(h, {h.var(0)}), {h.functor("app", {h.atom("nil"), h.var(0), h.var(0)})}, 1);
    const std::optional<pud_solution> solution = h.next_solution(conjecture, 10);
    ASSERT_TRUE(solution.has_value());
    EXPECT_EQ(solution->answer, q(h, {h.var(0)}));
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, LargeFrameOffsetsDoNotCollide) {
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
    const pud_node_id conjecture = h.start_conjecture(h.functor("q", query_args), {h.functor("tup", goal_args)}, 20);
    const std::optional<pud_solution> solution = h.next_solution(conjecture, 10);
    ASSERT_TRUE(solution.has_value());
    std::vector<const expr*> expected_args;
    for (uint32_t index = 0; index < 19; ++index)
        expected_args.push_back(h.var(index));
    expected_args.push_back(h.functor("f", {h.var(5), h.var(14)}));
    EXPECT_EQ(solution->answer, h.functor("q", expected_args));
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, LongAppendIsCorrect) {
    h.add_append();
    std::vector<const expr*> front;
    std::vector<const expr*> back;
    for (int index = 0; index < 50; ++index) {
        front.push_back(h.atom("a" + std::to_string(index)));
        back.push_back(h.atom("b" + std::to_string(index)));
    }
    std::vector<const expr*> both = front;
    both.insert(both.end(), back.begin(), back.end());
    const pud_node_id conjecture = h.start_conjecture(
        q(h, {h.var(0)}), {h.functor("app", {h.list(front), h.list(back), h.var(0)})}, 1);
    EXPECT_THAT(h.enumerate(conjecture, 5, 200), ElementsAre(q(h, {h.list(both)})));
    EXPECT_TRUE(h.exhausted(conjecture));
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, ManyQueriesInterleaved) {
    h.add_colors();
    h.add_nat();
    h.add_member();
    const pud_node_id colors = h.start_conjecture(h.functor("c", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    const pud_node_id numbers = h.start_conjecture(h.functor("n", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    const pud_node_id members = h.start_conjecture(
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
    EXPECT_THAT(member_answers, UnorderedElementsAre(h.functor("m", {h.atom("a")}), h.functor("m", {h.atom("b")})));
    ASSERT_GE(number_answers.size(), 5u);
    for (size_t index = 0; index < number_answers.size(); ++index)
        EXPECT_EQ(number_answers.at(index), h.functor("n", {h.peano(static_cast<uint32_t>(index))}));
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, VeryDeepTermsDoNotOverflowTheStack) {
    h.add_nat();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("nat", {h.peano(300000)})}, 0);
    std::vector<const expr*> answers;
    EXPECT_FALSE(h.refuted(conjecture));
    EXPECT_TRUE(h.step(conjecture, answers));
    EXPECT_FALSE(h.refuted(conjecture));
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, VeryDeepFactIsAcceptedAndAnswered) {
    h.axiom(h.functor("big", {h.peano(200000)}), {}, 0);
    const pud_node_id conjecture = h.start_conjecture(q(h, {h.var(0)}), {h.functor("big", {h.var(0)})}, 1);
    EXPECT_THAT(h.enumerate(conjecture, 5, 10), ElementsAre(q(h, {h.peano(200000)})));
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, VeryDeepTermsUnifyThroughAQuery) {
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    const pud_node_id same = h.start_conjecture(h.atom("q1"), {h.functor("eq", {h.peano(200000), h.peano(200000)})}, 0);
    const pud_node_id different = h.start_conjecture(
        h.atom("q2"), {h.functor("eq", {h.peano(200000), h.peano_on(200000, "bad")})}, 0);
    EXPECT_THAT(h.enumerate(same, 5, 10), ElementsAre(h.atom("q1")));
    EXPECT_TRUE(h.refuted(different));
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, VeryDeepOccursCheckRefutesThroughAQuery) {
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    const expr* cyclic = h.var(0);
    for (int level = 0; level < 200000; ++level)
        cyclic = h.functor("f", {cyclic});
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("eq", {h.var(0), cyclic})}, 1);
    EXPECT_TRUE(h.refuted(conjecture));
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, VeryDeepGoalIsWatchedAndForked) {
    d.add_colors();
    d.axiom(d.functor("deepwarm", {d.peano_on(200000, "red")}), {}, 0);
    const expr* chain = d.var(0);
    for (int level = 0; level < 200000; ++level)
        chain = d.functor("s", {chain});
    const pud_node_id conjecture = d.start_conjecture(
        q(d, {d.var(0)}), {d.functor("color", {d.var(0)}), d.functor("deepwarm", {chain})}, 1);
    const pud_mhq_head_id before_binding = d.query_of(conjecture, 1).value();
    d.expand(conjecture, 0);
    size_t live_children = 0;
    for (pud_node_id child : d.children_of(conjecture)) {
        if (d.refuted(child))
            continue;
        ++live_children;
        EXPECT_NE(d.query_of(child, 1), before_binding);
    }
    EXPECT_EQ(live_children, 1u);
    EXPECT_THAT(d.proven_answers(conjecture, 20), ElementsAre(q(d, {d.atom("red")})));
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, ManifestDestroyedMidSolveIsClean) {
    for (size_t round = 0; round < 25; ++round) {
        auto harness = std::make_unique<pud_manifest_harness>(static_cast<uint32_t>(round), pud_harness_scope::conjecture_leaves);
        harness->add_nat();
        harness->add_append();
        const pud_node_id numbers = harness->start_conjecture(
            harness->functor("n", {harness->var(0)}), {harness->functor("nat", {harness->var(0)})}, 1);
        const pud_node_id splits = harness->start_conjecture(
            harness->functor("s", {harness->var(0), harness->var(1)}),
            {harness->functor("app", {harness->var(0), harness->var(1), harness->list({harness->atom("x"), harness->atom("y")})})},
            2);
        harness->next_solution(numbers, round);
        harness->next_solution(splits, round / 2);
        harness->check_all();
        EXPECT_NO_THROW(harness.reset());
    }
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, AllQueriesGoneAfterRefutingEveryRoot) {
    h.add_colors();
    h.add_nat();
    h.add_append();
    const pud_node_id conjunction = h.start_conjecture(
        q(h, {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    const pud_node_id numbers = h.start_conjecture(h.functor("n", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    const pud_node_id splits = h.start_conjecture(
        h.functor("s", {h.var(0), h.var(1)}),
        {h.functor("app", {h.var(0), h.var(1), h.list({h.atom("x"), h.atom("y")})})},
        2);
    std::set<pud_mhq_head_id> seen;
    std::vector<const expr*> answers;
    for (int round = 0; round < 15; ++round) {
        for (pud_node_id query : {conjunction, numbers, splits}) {
            h.step(query, answers);
            for (pud_node_id leaf : h.open_leaves()) {
                for (const pud_body_goal& goal : h.open_goals(leaf))
                    seen.insert(h.query_of(leaf, goal.id).value());
            }
        }
    }
    ASSERT_FALSE(seen.empty());
    for (pud_node_id root : h.roots)
        h.refute(root);
    for (pud_mhq_head_id head : seen)
        EXPECT_FALSE(h.alive(head)) << "head " << head;
    EXPECT_TRUE(h.live_leaves().empty());
    h.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, AxiomTreeQueriesLiveOnlyWhileItsLeavesDo) {
    const pud_node_id rule = d.add_nat();
    d.open_axioms();
    d.grow_along_open_goals(rule, 3);
    std::vector<pud_mhq_head_id> heads;
    for (pud_node_id leaf : d.open_leaves()) {
        for (const pud_body_goal& goal : d.open_goals(leaf))
            heads.push_back(d.query_of(leaf, goal.id).value());
    }
    ASSERT_FALSE(heads.empty());
    for (pud_mhq_head_id head : heads)
        EXPECT_TRUE(d.alive(head));
    d.refute(rule);
    for (pud_mhq_head_id head : heads)
        EXPECT_FALSE(d.alive(head)) << "head " << head;
    d.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, SingleCandidateGoalIsReportedUnit) {
    h.add_colors();
    const pud_node_id unit = h.start_conjecture(h.atom("q1"), {h.functor("warm", {h.var(0)})}, 1);
    const pud_node_id live = h.start_conjecture(h.atom("q2"), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_THAT(h.manifest.unit_queries_, Contains(h.query_of(unit, 0).value()));
    EXPECT_THAT(h.manifest.unit_queries_, Not(Contains(h.query_of(live, 0).value())));
    EXPECT_EQ(candidate_paths(h, unit, 0).size(), 1u);
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, BudgetedSolveResumesWhereItStopped) {
    pud_manifest_harness whole{0, pud_harness_scope::conjecture_leaves};
    pud_manifest_harness stepped{0, pud_harness_scope::conjecture_leaves};
    whole.add_nat();
    stepped.add_nat();
    const pud_node_id whole_conjecture = whole.start_conjecture(whole.atom("q"), {whole.functor("nat", {whole.peano(6)})}, 0);
    const pud_node_id stepped_conjecture = stepped.start_conjecture(stepped.atom("q"), {stepped.functor("nat", {stepped.peano(6)})}, 0);
    const std::optional<pud_solution> at_once = whole.next_solution(whole_conjecture, 100);
    std::optional<pud_solution> one_by_one;
    for (int attempt = 0; attempt < 100 && !one_by_one.has_value(); ++attempt)
        one_by_one = stepped.next_solution(stepped_conjecture, 1);
    ASSERT_TRUE(at_once.has_value());
    ASSERT_TRUE(one_by_one.has_value());
    EXPECT_EQ(at_once->leaf, one_by_one->leaf);
    EXPECT_EQ(whole.all_nodes(), stepped.all_nodes());
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, SelfCandidateEnumeratesWithoutGaps) {
    d.axiom(d.functor("nat", {d.atom("z")}), {}, 0);
    const pud_node_id rule = d.axiom(d.functor("nat", {d.functor("s", {d.var(0)})}), {d.functor("nat", {d.var(0)})}, 1);
    d.open_axioms();
    for (int step = 0; step < 60 && d.proven_answers(rule, 0).size() < 12; ++step) {
        ASSERT_TRUE(d.advance_unfold_loop());
        if (step % 6 == 0)
            d.check_all();
    }
    const std::vector<const expr*> answers = d.proven_answers(rule, 0);
    for (uint32_t count = 1; count <= 6; ++count)
        EXPECT_NE(std::find(answers.begin(), answers.end(), d.functor("nat", {d.peano(count)})), answers.end()) << count;
    for (const expr* answer : answers) {
        bool small_number = false;
        for (uint32_t count = 1; count <= 64; ++count)
            small_number = small_number || answer == d.functor("nat", {d.peano(count)});
        EXPECT_TRUE(small_number) << "answers must not jump to huge numbers";
    }
    d.check_all();
}

TEST_F(PudManifestConjectureScenarioIntegrationTest, EnumeratedAxiomTreeStillServesAsALemma) {
    d.add_colors();
    const pud_node_id first = d.axiom(
        d.functor("warmcolor", {d.var(0)}), {d.functor("color", {d.var(0)}), d.functor("warm", {d.var(0)})}, 1);
    d.open_axioms();
    EXPECT_THAT(d.proven_answers(first, 50), ElementsAre(d.functor("warmcolor", {d.atom("red")})));
    EXPECT_THAT(d.open_leaves(), IsEmpty());
    EXPECT_FALSE(d.refuted(first));
    const pud_node_id second = d.start_conjecture(q(d, {d.var(0)}), {d.functor("warmcolor", {d.var(0)})}, 1);
    EXPECT_THAT(distinct(d.proven_answers(second, 50)), ElementsAre(q(d, {d.atom("red")})));
    d.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, YieldedSolutionStaysALiveLeafAndWitness) {
    d.add_colors();
    const pud_node_id first = d.axiom(d.functor("pick", {d.var(0)}), {d.functor("color", {d.var(0)})}, 1);
    d.open_axioms();
    d.expand(first, 0);
    ASSERT_FALSE(d.children_of(first).empty());
    const pud_node_id solution = d.children_of(first).front();
    EXPECT_TRUE(d.live_leaf(solution));
    EXPECT_THAT(d.open_goals(solution), IsEmpty());
    for (pud_node_id child : d.children_of(first)) {
        if (child != solution)
            d.refute(child);
    }
    const pud_node_id second = d.start_conjecture(q(d, {d.var(0)}), {d.functor("pick", {d.var(0)})}, 1);
    bool lands_on_the_solution = false;
    for (const std::vector<pud_node_id>& path : candidate_paths(d, second, 0))
        lands_on_the_solution = lands_on_the_solution || path == std::vector<pud_node_id>{first, solution};
    EXPECT_TRUE(lands_on_the_solution);
    const expr* picked = std::get<expr::functor>(
        d.manifest.leaf_materializer_.materialize(solution, d.manifest.root_heads_.get(first))->content).args.front();
    EXPECT_THAT(distinct(d.proven_answers(second, 50)), ElementsAre(q(d, {picked})));
    d.check_all();
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, ExhaustedSearchIsNotARefutation) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(q(h, {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_THAT(h.enumerate(conjecture, 10, 50), UnorderedElementsAre(q(h, {h.atom("red")}), q(h, {h.atom("blue")})));
    EXPECT_TRUE(h.exhausted(conjecture));
    EXPECT_FALSE(h.refuted(conjecture));
    EXPECT_FALSE(h.next_solution(conjecture, 10).has_value());
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, NoSolutionQueryEndsBothExhaustedAndRefuted) {
    h.add_colors();
    h.axiom(h.functor("cold", {h.atom("blue")}), {}, 0);
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("warm", {h.var(0)}), h.functor("cold", {h.var(0)})}, 1);
    EXPECT_THAT(h.enumerate(conjecture, 10, 50), IsEmpty());
    EXPECT_TRUE(h.exhausted(conjecture));
    EXPECT_TRUE(h.refuted(conjecture));
}


TEST_F(PudManifestConjectureScenarioIntegrationTest, AForkedQueryReplacesTheParentsAtTheChild) {
    d.add_colors();
    const pud_node_id conjecture = d.start_conjecture(
        q(d, {d.var(0)}), {d.functor("color", {d.var(0)}), d.functor("warm", {d.var(0)})}, 1);
    const pud_mhq_head_id color_query = d.query_of(conjecture, 0).value();
    d.expand(conjecture, 1);
    ASSERT_EQ(d.children_of(conjecture).size(), 1u);
    const pud_node_id child = d.children_of(conjecture).front();
    EXPECT_NE(d.query_of(child, 0).value(), color_query);
    d.check_all();
}
