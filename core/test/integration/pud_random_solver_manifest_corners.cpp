#include <algorithm>
#include <string>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_manifest_harness.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct PudRandomSolverManifestCornerTest : public ::testing::Test {
    pud_random_solver_manifest_harness h{0};

    const expr* q(std::vector<const expr*> args) { return h.functor("q", std::move(args)); }

    std::vector<const expr*> distinct(std::vector<const expr*> answers) {
        std::sort(answers.begin(), answers.end());
        answers.erase(std::unique(answers.begin(), answers.end()), answers.end());
        return answers;
    }
};

TEST_F(PudRandomSolverManifestCornerTest, ConjectureWithoutGoalsIsSolvedAtOnce) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {}, 0);
    EXPECT_EQ(h.next_solution(conjecture, 0), conjecture);
    EXPECT_TRUE(h.exhausted(conjecture));
    EXPECT_FALSE(h.refuted(conjecture));
    h.check_all();
}

TEST_F(PudRandomSolverManifestCornerTest, ConjectureWhoseOnlyCandidateIsRefutedIsRefuted) {
    const pud_node_id orphan = h.axiom(h.functor("p", {h.var(0)}), {h.functor("missing", {h.var(0)})}, 1);
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("p", {h.atom("a")})}, 0);
    EXPECT_TRUE(h.refuted(orphan));
    EXPECT_TRUE(h.refuted(conjecture));
    EXPECT_FALSE(h.next_solution(conjecture, 50).has_value());
    EXPECT_TRUE(h.exhausted(conjecture));
    h.check_all();
}

TEST_F(PudRandomSolverManifestCornerTest, VariablesSharedAcrossGoalsAndRepeatedInTheAnswer) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(
        q({h.var(0), h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    EXPECT_THAT(distinct(h.enumerate(conjecture, 10, 100)), ElementsAre(q({h.atom("red"), h.atom("red")})));
    EXPECT_TRUE(h.exhausted(conjecture));
    h.check_all();
}

TEST_F(PudRandomSolverManifestCornerTest, AnswersSplitAListBothWays) {
    h.add_append();
    const pud_node_id conjecture = h.start_conjecture(
        q({h.var(0), h.var(1)}), {h.functor("app", {h.var(0), h.var(1), h.list({h.atom("a")})})}, 2);
    EXPECT_THAT(distinct(h.enumerate(conjecture, 10, 200)), UnorderedElementsAre(
        q({h.atom("nil"), h.list({h.atom("a")})}), q({h.list({h.atom("a")}), h.atom("nil")})));
    EXPECT_TRUE(h.exhausted(conjecture));
    h.check_all();
}

TEST_F(PudRandomSolverManifestCornerTest, UnconstrainedAnswerVariableStaysAVariable) {
    h.axiom(h.functor("any", {h.var(0)}), {}, 1);
    const pud_node_id conjecture = h.start_conjecture(q({h.var(0)}), {h.functor("any", {h.var(0)})}, 1);
    const std::vector<const expr*> answers = h.enumerate(conjecture, 10, 50);
    ASSERT_EQ(answers.size(), 1u);
    const expr* argument = std::get<expr::functor>(answers.front()->content).args.front();
    EXPECT_TRUE(std::holds_alternative<expr::var>(argument->content));
    h.check_all();
}

TEST_F(PudRandomSolverManifestCornerTest, RuleWithoutABaseNeverSolvesButStaysLive) {
    h.axiom(h.atom("p"), {h.atom("p")}, 0);
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.atom("p")}, 0);
    EXPECT_FALSE(h.next_solution(conjecture, 50).has_value());
    EXPECT_FALSE(h.exhausted(conjecture));
    EXPECT_FALSE(h.refuted(conjecture));
    h.check_all();
}

TEST_F(PudRandomSolverManifestCornerTest, FiniteConjectureEndsWhileAnInfiniteOneContinues) {
    h.add_colors();
    h.add_nat();
    const pud_node_id colors = h.start_conjecture(q({h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    const pud_node_id numbers = h.start_conjecture(h.functor("n", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    EXPECT_THAT(distinct(h.enumerate(colors, 10, 200)), UnorderedElementsAre(q({h.atom("red")}), q({h.atom("blue")})));
    EXPECT_TRUE(h.exhausted(colors));
    EXPECT_EQ(h.enumerate(numbers, 5, 200).size(), 5u);
    EXPECT_FALSE(h.exhausted(numbers));
    h.check_all();
}

TEST_F(PudRandomSolverManifestCornerTest, ConjectureStartedAfterTheLoopEndedIsStillSolved) {
    h.add_colors();
    const pud_node_id first = h.start_conjecture(q({h.var(0)}), {h.functor("warm", {h.var(0)})}, 1);
    EXPECT_THAT(h.enumerate(first, 10, 100), ElementsAre(q({h.atom("red")})));
    EXPECT_TRUE(h.exhausted(first));
    const pud_node_id second = h.start_conjecture(q({h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_THAT(distinct(h.enumerate(second, 10, 100)), UnorderedElementsAre(q({h.atom("red")}), q({h.atom("blue")})));
    h.check_all();
}

TEST_F(PudRandomSolverManifestCornerTest, ConjectureShapedLikeAnAxiomHeadIsNeverACandidate) {
    h.add_colors();
    h.start_conjecture(h.functor("pair", {h.var(0), h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    const pud_node_id user = h.start_conjecture(h.atom("u"), {h.functor("pair", {h.atom("red"), h.atom("red")})}, 0);
    EXPECT_TRUE(h.refuted(user));
    h.check_all();
}
