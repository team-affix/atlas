#include <algorithm>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_random_solver_manifest_harness.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct PudRandomSolverManifestScenarioTest : public ::testing::Test {
    pud_random_solver_manifest_harness h{0};

    const expr* q(std::vector<const expr*> args) { return h.functor("q", std::move(args)); }

    std::vector<const expr*> distinct(std::vector<const expr*> answers) {
        std::sort(answers.begin(), answers.end());
        answers.erase(std::unique(answers.begin(), answers.end()), answers.end());
        return answers;
    }

    bool solvable(pud_random_solver_manifest_harness& harness, const expr* goal, size_t budget) {
        const pud_node_id conjecture = harness.start_conjecture(harness.atom("q"), {goal}, 0);
        return harness.next_solution(conjecture, budget).has_value();
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
        h.run_base_descent();
        h.manifest.unfolder_.unfold(p_rule, 0);
        EXPECT_EQ(h.children_of(p_rule).size(), 1u);
        return h.children_of(p_rule).front();
    }
};

// ---------- sharing and renames ----------

TEST_F(PudRandomSolverManifestScenarioTest, AcceptingASharedQueryAtADeepDescendantSucceeds) {
    h.add_colors();
    h.axiom(h.atom("a1"), {}, 0);
    h.axiom(h.atom("a2"), {}, 0);
    h.axiom(h.atom("a3"), {}, 0);
    const pud_node_id conjecture = h.start_conjecture(
        q({h.var(0)}), {h.atom("a1"), h.atom("a2"), h.atom("a3"), h.functor("color", {h.var(0)})}, 1);
    const pud_mhq_head_id shared = h.query_of(conjecture, 3).value();
    pud_node_id frontier = conjecture;
    for (body_goal_id goal = 0; goal < 3; ++goal) {
        h.manifest.unfolder_.unfold(frontier, goal);
        ASSERT_EQ(h.children_of(frontier).size(), 1u);
        frontier = h.children_of(frontier).front();
        EXPECT_EQ(h.query_of(frontier, 3), shared);
    }
    EXPECT_EQ(h.manifest.query_head_nodes_.get(shared), conjecture);
    EXPECT_NO_THROW(h.manifest.unfolder_.unfold(frontier, 3));
    EXPECT_EQ(h.children_of(frontier).size(), 2u);
    h.check_all();
    EXPECT_THAT(h.enumerate(conjecture, 10, 50), UnorderedElementsAre(q({h.atom("red")}), q({h.atom("blue")})));
}

TEST_F(PudRandomSolverManifestScenarioTest, CrossGoalVariableMergeIsHandledAtAcceptance) {
    h.add_colors();
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    const pud_node_id conjecture = h.start_conjecture(
        q({h.var(0), h.var(1)}),
        {h.functor("eq", {h.var(0), h.var(1)}), h.functor("color", {h.var(0)}), h.functor("color", {h.var(1)})},
        2);
    std::vector<const expr*> answers;
    while (h.step(conjecture, answers))
        h.check_all();
    EXPECT_THAT(
        answers,
        UnorderedElementsAre(q({h.atom("red"), h.atom("red")}), q({h.atom("blue"), h.atom("blue")})));
}

TEST_F(PudRandomSolverManifestScenarioTest, RenameOnlyStepsDoNotForkButStayCorrect) {
    h.add_colors();
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    const pud_node_id conjecture = h.start_conjecture(
        q({h.var(0), h.var(1)}),
        {h.functor("eq", {h.var(0), h.var(1)}), h.functor("color", {h.var(0)}), h.functor("color", {h.var(1)})},
        2);
    const pud_mhq_head_id first_color = h.query_of(conjecture, 1).value();
    const pud_mhq_head_id second_color = h.query_of(conjecture, 2).value();
    h.manifest.unfolder_.unfold(conjecture, 0);
    ASSERT_EQ(h.children_of(conjecture).size(), 1u);
    const pud_node_id merged = h.children_of(conjecture).front();
    EXPECT_EQ(h.query_of(merged, 1), first_color);
    EXPECT_EQ(h.query_of(merged, 2), second_color);
    EXPECT_THAT(
        h.manifest.node_added_substitutions_.get(merged),
        ElementsAre(pud_substitution{.var_idx = 1, .value = h.var(0)}));
    h.check_all();
    EXPECT_THAT(
        h.enumerate(conjecture, 10, 50),
        UnorderedElementsAre(q({h.atom("red"), h.atom("red")}), q({h.atom("blue"), h.atom("blue")})));
}

TEST_F(PudRandomSolverManifestScenarioTest, ForkedQueriesOnlyListCandidatesThatStillUnify) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(
        q({h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    h.manifest.unfolder_.unfold(conjecture, 0);
    size_t live_children = 0;
    for (pud_node_id child : h.children_of(conjecture)) {
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

TEST_F(PudRandomSolverManifestScenarioTest, AxiomTreeIsItsOwnCandidate) {
    const pud_node_id fact = h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
    const pud_node_id rule = h.axiom(
        h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
    h.run_base_descent();
    EXPECT_THAT(
        candidate_paths(rule, 0),
        UnorderedElementsAre(std::vector<pud_node_id>{fact}, std::vector<pud_node_id>{rule}));
    const pud_node_id conjecture = h.start_conjecture(q({h.var(0)}), {h.functor("nat", {h.functor("s", {h.var(0)})})}, 1);
    const std::optional<pud_node_id> solution = h.next_solution(conjecture, 10);
    ASSERT_TRUE(solution.has_value());
    EXPECT_EQ(h.answer(conjecture, solution.value()), q({h.atom("z")}));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, ExpandingTheLeafThatIsItsOwnGoalsCandidate) {
    h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
    const pud_node_id rule = h.axiom(
        h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
    h.run_base_descent();
    EXPECT_NO_THROW(h.manifest.unfolder_.unfold(rule, 0));
    EXPECT_EQ(h.children_of(rule).size(), 2u);
    h.check_all();
    for (uint32_t count = 1; count <= 6; ++count)
        EXPECT_TRUE(solvable(h, h.functor("nat", {h.peano(count)}), 100)) << count;
    EXPECT_FALSE(solvable(h, h.functor("nat", {h.peano_on(3, "bad")}), 100));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, OneQueryUsesAnAxiomRuleAsALemma) {
    h.add_colors();
    const pud_node_id warmcolor = h.axiom(
        h.functor("warmcolor", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    h.run_base_descent();
    const pud_node_id conjecture = h.start_conjecture(q({h.var(0)}), {h.functor("warmcolor", {h.var(0)})}, 1);
    bool uses_rule = false;
    for (const std::vector<pud_node_id>& path : candidate_paths(conjecture, 0))
        uses_rule = uses_rule || path.front() == warmcolor;
    EXPECT_TRUE(uses_rule);
    EXPECT_THAT(distinct(h.enumerate(conjecture, 10, 50)), ElementsAre(q({h.atom("red")})));
    h.check_all();
}

// ---------- conjectures stay outside the database ----------

TEST_F(PudRandomSolverManifestScenarioTest, ConjectureIsNeverACandidateForAnotherConjecture) {
    h.add_colors();
    h.start_conjecture(
        h.functor("warmcolor", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    const pud_node_id second = h.start_conjecture(q({h.var(0)}), {h.functor("warmcolor", {h.var(0)})}, 1);
    EXPECT_TRUE(h.refuted(second));
    EXPECT_THAT(h.enumerate(second, 10, 50), IsEmpty());
    EXPECT_TRUE(h.exhausted(second));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, ConjectureIsNeverACandidateForItsOwnGoals) {
    const pud_node_id fact = h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
    const pud_node_id conjecture = h.start_conjecture(
        h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
    EXPECT_THAT(candidate_paths(conjecture, 0), ElementsAre(std::vector<pud_node_id>{fact}));
    EXPECT_THAT(distinct(h.enumerate(conjecture, 10, 50)),
        ElementsAre(h.functor("nat", {h.functor("s", {h.atom("z")})})));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, ConjectureNeverAddsAnswersToTheDatabase) {
    h.add_colors();
    h.start_conjecture(h.functor("color", {h.atom("green")}), {}, 0);
    h.start_conjecture(h.functor("color", {h.var(0)}), {h.functor("warm", {h.var(0)})}, 1);
    const pud_node_id colors = h.start_conjecture(q({h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_THAT(distinct(h.enumerate(colors, 10, 50)),
        UnorderedElementsAre(q({h.atom("red")}), q({h.atom("blue")})));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, ConjectureIsARootLeafButNotAnAxiom) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(q({h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_TRUE(h.root(conjecture));
    EXPECT_THAT(h.manifest.axioms_.get(), ::testing::Not(::testing::Contains(conjecture)));
    EXPECT_THROW(h.manifest.node_heads_.get(conjecture), std::out_of_range);
}

TEST_F(PudRandomSolverManifestScenarioTest, ConcurrentConjecturesNeverSeeEachOther) {
    h.add_colors();
    const pud_node_id first = h.start_conjecture(q({h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    const pud_node_id second = h.start_conjecture(q({h.var(0)}), {h.functor("warm", {h.var(0)})}, 1);
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
    EXPECT_THAT(distinct(first_answers), UnorderedElementsAre(q({h.atom("red")}), q({h.atom("blue")})));
    EXPECT_THAT(distinct(second_answers), ElementsAre(q({h.atom("red")})));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, CandidatesMoveDeeperWhenAnAxiomTreeGrows) {
    const pud_node_id rule = h.add_nat();
    h.run_base_descent();
    h.grow_along_open_goals(rule, 3);
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("nat", {h.peano(3)})}, 0);
    size_t deepest = 0;
    for (const std::vector<pud_node_id>& path : candidate_paths(conjecture, 0))
        deepest = std::max(deepest, path.size());
    EXPECT_GE(deepest, 3u);
    h.check_all();
    EXPECT_TRUE(h.next_solution(conjecture, 3).has_value());
}

TEST_F(PudRandomSolverManifestScenarioTest, LemmaPathReplayMatchesFlatResolution) {
    pud_random_solver_manifest_harness flat{0};
    flat.add_nat();
    const pud_node_id rule = h.add_nat();
    h.run_base_descent();
    h.grow_along_open_goals(rule, 4);
    for (uint32_t count = 0; count <= 8; ++count) {
        EXPECT_TRUE(solvable(flat, flat.functor("nat", {flat.peano(count)}), 200)) << count;
        EXPECT_TRUE(solvable(h, h.functor("nat", {h.peano(count)}), 200)) << count;
    }
    EXPECT_FALSE(solvable(flat, flat.functor("nat", {flat.peano_on(2, "bad")}), 200));
    EXPECT_FALSE(solvable(h, h.functor("nat", {h.peano_on(2, "bad")}), 200));
    const pud_node_id enumerating = h.start_conjecture(q({h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    const std::vector<const expr*> answers = distinct(h.enumerate(enumerating, 20, 300));
    for (uint32_t count = 0; count < 4; ++count)
        EXPECT_NE(std::find(answers.begin(), answers.end(), q({h.peano(count)})), answers.end()) << count;
    h.check_all();
}

// ---------- refutation cascades ----------

TEST_F(PudRandomSolverManifestScenarioTest, RefutingALemmaSubtreeRefutesQueriesThatOnlyHadIt) {
    const pud_node_id lemma = add_lemma_through_p();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("p", {h.atom("a")})}, 0);
    ASSERT_THAT(candidate_paths(conjecture, 0), ElementsAre(h.path_to(lemma)));
    h.manifest.node_refuter_.refute(lemma);
    EXPECT_TRUE(h.refuted(conjecture));
    EXPECT_FALSE(h.next_solution(conjecture, 50).has_value());
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, RefutingOneOfTwoCandidatesMakesTheQueryUnit) {
    const pud_node_id red = h.axiom(h.functor("color", {h.atom("red")}), {}, 0);
    h.axiom(h.functor("color", {h.atom("blue")}), {}, 0);
    const pud_node_id conjecture = h.start_conjecture(q({h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    ASSERT_EQ(candidate_paths(conjecture, 0).size(), 2u);
    h.manifest.node_refuter_.refute(red);
    EXPECT_EQ(candidate_paths(conjecture, 0).size(), 1u);
    EXPECT_FALSE(h.refuted(conjecture));
    EXPECT_THAT(h.enumerate(conjecture, 10, 50), ElementsAre(q({h.atom("blue")})));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, AllChildrenRefutedOnArrivalRefuteTheParent) {
    h.add_colors();
    h.axiom(h.functor("cold", {h.atom("green")}), {}, 0);
    const pud_node_id conjecture = h.start_conjecture(
        h.atom("q"), {h.functor("color", {h.var(0)}), h.functor("cold", {h.var(0)})}, 1);
    ASSERT_FALSE(h.refuted(conjecture));
    h.manifest.unfolder_.unfold(conjecture, 0);
    EXPECT_EQ(h.children_of(conjecture).size(), 2u);
    for (pud_node_id child : h.children_of(conjecture))
        EXPECT_TRUE(h.refuted(child));
    EXPECT_TRUE(h.refuted(conjecture));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, CascadeCrossesTrees) {
    const pud_node_id lemma = add_lemma_through_p();
    const pud_node_id first = h.start_conjecture(h.atom("q1"), {h.functor("p", {h.atom("a")})}, 0);
    const pud_node_id second = h.start_conjecture(h.atom("q2"), {h.functor("p", {h.atom("a")})}, 0);
    const pud_node_id independent = h.start_conjecture(h.atom("q3"), {h.functor("r", {h.atom("a")})}, 0);
    h.manifest.node_refuter_.refute(lemma);
    EXPECT_TRUE(h.refuted(first));
    EXPECT_TRUE(h.refuted(second));
    EXPECT_FALSE(h.refuted(independent));
    EXPECT_TRUE(h.next_solution(independent, 10).has_value());
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, RefutingAnAlreadyRefutedNodeIsANoop) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(q({h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    h.manifest.unfolder_.unfold(conjecture, 0);
    const pud_node_id child = h.children_of(conjecture).front();
    h.manifest.node_refuter_.refute(child);
    EXPECT_NO_THROW(h.manifest.node_refuter_.refute(child));
    EXPECT_NO_THROW(h.manifest.node_refuter_.refute(child));
    EXPECT_FALSE(h.refuted(conjecture));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, RefutedRootStaysUnsolvable) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(q({h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    h.manifest.node_refuter_.refute(conjecture);
    for (int attempt = 0; attempt < 3; ++attempt)
        EXPECT_FALSE(h.next_solution(conjecture, 50).has_value());
    EXPECT_NO_THROW(h.manifest.node_refuter_.refute(conjecture));
    h.check_all();
}

// ---------- unification edge cases ----------

TEST_F(PudRandomSolverManifestScenarioTest, OccursCheckRefutesCyclicGoals) {
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("eq", {h.var(0), h.functor("f", {h.var(0)})})}, 1);
    EXPECT_TRUE(h.refuted(conjecture));
    EXPECT_FALSE(h.next_solution(conjecture, 10).has_value());
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, RepeatedVariablesInHeadsAreRespected) {
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

TEST_F(PudRandomSolverManifestScenarioTest, GroundGoalsNeedNoSubstitutions) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("color", {h.atom("red")})}, 0);
    h.manifest.unfolder_.unfold(conjecture, 0);
    ASSERT_EQ(h.children_of(conjecture).size(), 1u);
    const pud_node_id child = h.children_of(conjecture).front();
    EXPECT_THAT(h.manifest.node_added_substitutions_.get(child), IsEmpty());
    EXPECT_EQ(h.manifest.node_added_var_count_.get(child), 0u);
    EXPECT_THAT(h.manifest.node_added_body_goals_.get(child), IsEmpty());
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, AnswerKeepsUnboundVariables) {
    h.add_append();
    const pud_node_id conjecture = h.start_conjecture(q({h.var(0)}), {h.functor("app", {h.atom("nil"), h.var(0), h.var(0)})}, 1);
    const std::optional<pud_node_id> solution = h.next_solution(conjecture, 10);
    ASSERT_TRUE(solution.has_value());
    EXPECT_EQ(h.answer(conjecture, solution.value()), q({h.var(0)}));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, LargeFrameOffsetsDoNotCollide) {
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
    const std::optional<pud_node_id> solution = h.next_solution(conjecture, 10);
    ASSERT_TRUE(solution.has_value());
    std::vector<const expr*> expected_args;
    for (uint32_t index = 0; index < 19; ++index)
        expected_args.push_back(h.var(index));
    expected_args.push_back(h.functor("f", {h.var(5), h.var(14)}));
    EXPECT_EQ(h.answer(conjecture, solution.value()), h.functor("q", expected_args));
    h.check_all();
}

// ---------- scale ----------

TEST_F(PudRandomSolverManifestScenarioTest, LongAppendIsCorrect) {
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
        q({h.var(0)}), {h.functor("app", {h.list(front), h.list(back), h.var(0)})}, 1);
    EXPECT_THAT(h.enumerate(conjecture, 5, 200), ElementsAre(q({h.list(both)})));
    EXPECT_TRUE(h.exhausted(conjecture));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, ManyQueriesInterleaved) {
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
    EXPECT_THAT(member_answers, ElementsAre(h.functor("m", {h.atom("a")}), h.functor("m", {h.atom("b")})));
    ASSERT_GE(number_answers.size(), 5u);
    for (size_t index = 0; index < number_answers.size(); ++index)
        EXPECT_EQ(number_answers.at(index), h.functor("n", {h.peano(static_cast<uint32_t>(index))}));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, VeryDeepTermsDoNotOverflowTheStack) {
    h.add_nat();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("nat", {h.peano(300000)})}, 0);
    std::vector<const expr*> answers;
    EXPECT_FALSE(h.refuted(conjecture));
    EXPECT_TRUE(h.step(conjecture, answers));
    EXPECT_FALSE(h.refuted(conjecture));
}

TEST_F(PudRandomSolverManifestScenarioTest, VeryDeepFactIsAcceptedAndAnswered) {
    h.axiom(h.functor("big", {h.peano(200000)}), {}, 0);
    const pud_node_id conjecture = h.start_conjecture(q({h.var(0)}), {h.functor("big", {h.var(0)})}, 1);
    EXPECT_THAT(h.enumerate(conjecture, 5, 10), ElementsAre(q({h.peano(200000)})));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, VeryDeepTermsUnifyThroughAQuery) {
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    const pud_node_id same = h.start_conjecture(h.atom("q1"), {h.functor("eq", {h.peano(200000), h.peano(200000)})}, 0);
    const pud_node_id different = h.start_conjecture(
        h.atom("q2"), {h.functor("eq", {h.peano(200000), h.peano_on(200000, "bad")})}, 0);
    EXPECT_THAT(h.enumerate(same, 5, 10), ElementsAre(h.atom("q1")));
    EXPECT_TRUE(h.refuted(different));
}

TEST_F(PudRandomSolverManifestScenarioTest, VeryDeepOccursCheckRefutesThroughAQuery) {
    h.axiom(h.functor("eq", {h.var(0), h.var(0)}), {}, 1);
    const expr* cyclic = h.var(0);
    for (int level = 0; level < 200000; ++level)
        cyclic = h.functor("f", {cyclic});
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("eq", {h.var(0), cyclic})}, 1);
    EXPECT_TRUE(h.refuted(conjecture));
}

TEST_F(PudRandomSolverManifestScenarioTest, VeryDeepGoalIsWatchedAndForked) {
    h.add_colors();
    h.axiom(h.functor("deepwarm", {h.peano_on(200000, "red")}), {}, 0);
    const expr* chain = h.var(0);
    for (int level = 0; level < 200000; ++level)
        chain = h.functor("s", {chain});
    const pud_node_id conjecture = h.start_conjecture(
        q({h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("deepwarm", {chain})}, 1);
    const pud_mhq_head_id before_binding = h.query_of(conjecture, 1).value();
    h.manifest.unfolder_.unfold(conjecture, 0);
    size_t live_children = 0;
    for (pud_node_id child : h.children_of(conjecture)) {
        if (h.refuted(child))
            continue;
        ++live_children;
        EXPECT_NE(h.query_of(child, 1), before_binding);
    }
    EXPECT_EQ(live_children, 1u);
    EXPECT_THAT(h.enumerate(conjecture, 5, 20), ElementsAre(q({h.atom("red")})));
}

// ---------- lifetime and destruction ----------

TEST_F(PudRandomSolverManifestScenarioTest, ManifestDestroyedMidSolveIsClean) {
    for (size_t round = 0; round < 25; ++round) {
        auto harness = std::make_unique<pud_random_solver_manifest_harness>(static_cast<uint32_t>(round));
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

TEST_F(PudRandomSolverManifestScenarioTest, AllQueriesGoneAfterRefutingEveryRoot) {
    h.add_colors();
    h.add_nat();
    h.add_append();
    const pud_node_id conjunction = h.start_conjecture(
        q({h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
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

TEST_F(PudRandomSolverManifestScenarioTest, AxiomTreeQueriesLiveOnlyWhileItsLeavesDo) {
    const pud_node_id rule = h.add_nat();
    h.run_base_descent();
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

TEST_F(PudRandomSolverManifestScenarioTest, SingleCandidateGoalIsReportedUnit) {
    h.add_colors();
    const pud_node_id unit = h.start_conjecture(h.atom("q1"), {h.functor("warm", {h.var(0)})}, 1);
    const pud_node_id live = h.start_conjecture(h.atom("q2"), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_THAT(h.manifest.node_descents_.get(unit).body_goal_pq().unit_goals(), ElementsAre(0u));
    EXPECT_THAT(h.manifest.node_descents_.get(live).body_goal_pq().unit_goals(), IsEmpty());
    EXPECT_EQ(candidate_paths(unit, 0).size(), 1u);
}

TEST_F(PudRandomSolverManifestScenarioTest, BudgetedSolveResumesWhereItStopped) {
    pud_random_solver_manifest_harness whole{0};
    pud_random_solver_manifest_harness stepped{0};
    whole.add_nat();
    stepped.add_nat();
    const pud_node_id whole_conjecture = whole.start_conjecture(whole.atom("q"), {whole.functor("nat", {whole.peano(6)})}, 0);
    const pud_node_id stepped_conjecture = stepped.start_conjecture(stepped.atom("q"), {stepped.functor("nat", {stepped.peano(6)})}, 0);
    const std::optional<pud_node_id> at_once = whole.next_solution(whole_conjecture, 100);
    std::optional<pud_node_id> one_by_one;
    for (int attempt = 0; attempt < 100 && !one_by_one.has_value(); ++attempt)
        one_by_one = stepped.next_solution(stepped_conjecture, 1);
    ASSERT_TRUE(at_once.has_value());
    ASSERT_TRUE(one_by_one.has_value());
    EXPECT_EQ(at_once, one_by_one);
    EXPECT_EQ(whole.manifest.node_id_sequencer_.peek(), stepped.manifest.node_id_sequencer_.peek());
}

// ---------- yielded solutions are reported, never refuted ----------

TEST_F(PudRandomSolverManifestScenarioTest, SelfCandidateEnumeratesWithoutGaps) {
    h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
    const pud_node_id query = h.axiom(
        h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
    h.run_base_descent();
    std::vector<const expr*> answers;
    for (int step = 0; step < 60 && answers.size() < 12; ++step) {
        ASSERT_TRUE(h.step(query, answers));
        if (step % 6 == 0)
            h.check_all();
    }
    const std::vector<const expr*> found = distinct(answers);
    for (uint32_t count = 1; count <= 6; ++count)
        EXPECT_NE(std::find(found.begin(), found.end(), h.functor("nat", {h.peano(count)})), found.end()) << count;
    for (const expr* answer : answers) {
        bool small_number = false;
        for (uint32_t count = 1; count <= 64; ++count)
            small_number = small_number || answer == h.functor("nat", {h.peano(count)});
        EXPECT_TRUE(small_number) << "answers must not jump to huge numbers";
    }
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, EnumeratedAxiomTreeStillServesAsALemma) {
    h.add_colors();
    const pud_node_id first = h.axiom(
        h.functor("warmcolor", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    h.run_base_descent();
    EXPECT_THAT(h.enumerate(first, 10, 50), ElementsAre(h.functor("warmcolor", {h.atom("red")})));
    EXPECT_TRUE(h.exhausted(first));
    EXPECT_FALSE(h.refuted(first));
    const pud_node_id second = h.start_conjecture(q({h.var(0)}), {h.functor("warmcolor", {h.var(0)})}, 1);
    EXPECT_THAT(distinct(h.enumerate(second, 10, 50)), ElementsAre(q({h.atom("red")})));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, YieldedSolutionStaysALiveLeafAndWitness) {
    h.add_colors();
    const pud_node_id first = h.axiom(h.functor("pick", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    h.run_base_descent();
    const std::optional<pud_node_id> solution = h.next_solution(first, 10);
    ASSERT_TRUE(solution.has_value());
    EXPECT_TRUE(h.leaf(solution.value()));
    EXPECT_FALSE(h.refuted(solution.value()));
    EXPECT_TRUE(h.has_descent(solution.value()));
    for (pud_node_id child : h.children_of(first)) {
        if (child != solution.value())
            h.manifest.node_refuter_.refute(child);
    }
    const pud_node_id second = h.start_conjecture(q({h.var(0)}), {h.functor("pick", {h.var(0)})}, 1);
    bool lands_on_the_solution = false;
    for (const std::vector<pud_node_id>& path : candidate_paths(second, 0))
        lands_on_the_solution = lands_on_the_solution || path == std::vector<pud_node_id>{first, solution.value()};
    EXPECT_TRUE(lands_on_the_solution);
    const expr* picked = std::get<expr::functor>(h.answer(first, solution.value())->content).args.front();
    EXPECT_THAT(distinct(h.enumerate(second, 10, 50)), ElementsAre(q({picked})));
    h.check_all();
}

TEST_F(PudRandomSolverManifestScenarioTest, ExhaustedSearchIsNotARefutation) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(q({h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_THAT(h.enumerate(conjecture, 10, 50), UnorderedElementsAre(q({h.atom("red")}), q({h.atom("blue")})));
    EXPECT_TRUE(h.exhausted(conjecture));
    EXPECT_FALSE(h.refuted(conjecture));
    EXPECT_FALSE(h.next_solution(conjecture, 10).has_value());
}

TEST_F(PudRandomSolverManifestScenarioTest, NoSolutionQueryEndsBothExhaustedAndRefuted) {
    h.add_colors();
    h.axiom(h.functor("cold", {h.atom("blue")}), {}, 0);
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("warm", {h.var(0)}), h.functor("cold", {h.var(0)})}, 1);
    EXPECT_THAT(h.enumerate(conjecture, 10, 50), IsEmpty());
    EXPECT_TRUE(h.exhausted(conjecture));
    EXPECT_TRUE(h.refuted(conjecture));
}
