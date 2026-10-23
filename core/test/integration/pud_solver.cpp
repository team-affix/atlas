#include <algorithm>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_datalog_oracle.hpp"
#include "pud_expr_structure.hpp"
#include "pud_manifest_harness.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

namespace {

struct PudSolverIntegrationTest : public ::testing::Test {
    pud_manifest_harness h{0, pud_harness_scope::conjecture_leaves};

    std::optional<pud_unfold> next_unfold() {
        const std::optional<pud_solver_step> step = h.next_solver_step();
        if (!step.has_value() || !std::holds_alternative<pud_unfold>(step.value()))
            return std::nullopt;
        return std::get<pud_unfold>(step.value());
    }
};

std::set<std::vector<int>> read_answers(pud_manifest_harness& harness, const std::vector<const expr*>& answers) {
    std::set<std::vector<int>> read;
    for (const expr* answer : answers)
        read.insert(datalog_read_answer(harness, answer).value());
    return read;
}

}

TEST_F(PudSolverIntegrationTest, AFiniteConjectureCompletesWhileAnInfiniteOneContinues) {
    h.add_colors();
    h.add_nat();
    const pud_node_id numbers = h.start_conjecture(h.functor("n", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    const pud_node_id colors = h.start_conjecture(h.functor("c", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_THAT(h.enumerate(colors, 10, 100), UnorderedElementsAre(h.functor("c", {h.atom("red")}), h.functor("c", {h.atom("blue")})));
    EXPECT_TRUE(h.exhausted(colors));
    EXPECT_FALSE(h.exhausted(numbers));
}

TEST_F(PudSolverIntegrationTest, AConjectureAddedAfterSolvingEndedIsStillSolved) {
    h.add_colors();
    const pud_node_id first = h.start_conjecture(h.functor("c", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    h.enumerate(first, 10, 100);
    EXPECT_FALSE(h.advance_solver());
    const pud_node_id second = h.start_conjecture(h.functor("w", {h.var(0)}), {h.functor("warm", {h.var(0)})}, 1);
    EXPECT_THAT(h.enumerate(second, 10, 100), UnorderedElementsAre(h.functor("w", {h.atom("red")})));
}

TEST_F(PudSolverIntegrationTest, ARuleWithoutABaseNeverSolvesButNeverEnds) {
    h.axiom(h.atom("p"), {h.atom("p")}, 0);
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.atom("p")}, 0);
    for (int step = 0; step < 50; ++step)
        ASSERT_TRUE(h.advance_solver());
    EXPECT_FALSE(h.next_solution(conjecture, 0).has_value());
    EXPECT_FALSE(h.exhausted(conjecture));
}

TEST_F(PudSolverIntegrationTest, TheSameSeedSolvesTheSameWay) {
    pud_manifest_harness other{0, pud_harness_scope::conjecture_leaves};
    for (pud_manifest_harness* each : {&h, &other}) {
        each->add_append();
        each->add_member();
        each->start_conjecture(
            each->functor("q", {each->var(0), each->var(1)}),
            {each->functor("app", {each->var(0), each->var(1), each->list({each->atom("a"), each->atom("b"), each->atom("c")})}),
             each->functor("mem", {each->atom("b"), each->var(0)})},
            2);
    }
    for (int step = 0; step < 60; ++step) {
        const std::optional<pud_solver_step> mine = h.next_solver_step();
        const std::optional<pud_solver_step> theirs = other.next_solver_step();
        ASSERT_EQ(mine.has_value(), theirs.has_value());
        if (!mine.has_value())
            break;
        ASSERT_EQ(mine->index(), theirs->index());
        if (const pud_unfold* unfold = std::get_if<pud_unfold>(&mine.value())) {
            EXPECT_EQ(*unfold, std::get<pud_unfold>(theirs.value()));
            continue;
        }
        EXPECT_TRUE(same_structure(std::get<pud_solution>(mine.value()).answer, std::get<pud_solution>(theirs.value()).answer));
    }
}

TEST_F(PudSolverIntegrationTest, EnumerationProducesNoDuplicates) {
    h.add_append();
    const pud_node_id conjecture = h.start_conjecture(
        h.functor("q", {h.var(0), h.var(1)}),
        {h.functor("app", {h.var(0), h.var(1), h.list({h.atom("a"), h.atom("b"), h.atom("c"), h.atom("d")})})},
        2);
    const std::vector<const expr*> answers = h.enumerate(conjecture, 20, 200);
    EXPECT_EQ(answers.size(), 5u);
    EXPECT_EQ(std::set<const expr*>(answers.begin(), answers.end()).size(), answers.size());
    EXPECT_TRUE(h.exhausted(conjecture));
}

TEST_F(PudSolverIntegrationTest, FlatDatalogAnswersMatchABottomUpFixpoint) {
    for (unsigned seed = 0; seed < 80; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        pud_manifest_harness harness{seed, pud_harness_scope::conjecture_leaves};
        const datalog_program program = generate_datalog(seed, false);
        const pud_node_id conjecture = load_datalog(harness, program);
        const std::vector<const expr*> answers = harness.enumerate(conjecture, 1000, 5000);
        EXPECT_TRUE(harness.exhausted(conjecture));
        EXPECT_EQ(read_answers(harness, answers), datalog_answers(program));
    }
}

TEST_F(PudSolverIntegrationTest, EveryAnswerOfARecursiveProgramIsEntailed) {
    for (unsigned seed = 0; seed < 80; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        pud_manifest_harness harness{seed, pud_harness_scope::conjecture_leaves};
        const datalog_program program = generate_datalog(seed, true);
        const pud_node_id conjecture = load_datalog(harness, program);
        std::vector<const expr*> answers;
        for (int step = 0; step < 60 && harness.most_open_goals() <= 64 && harness.step(conjecture, answers); ++step) {}
        const std::set<std::vector<int>> entailed = datalog_answers(program);
        for (const std::vector<int>& answer : read_answers(harness, answers))
            EXPECT_TRUE(entailed.contains(answer));
    }
}

TEST_F(PudSolverIntegrationTest, AUnitGoalIsUnfoldedBeforeAnyChoice) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(
        h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    EXPECT_EQ(next_unfold(), (pud_unfold{.leaf = conjecture, .goal = 1}));
}

TEST_F(PudSolverIntegrationTest, ForcedUnfoldsReachAxiomLeavesNoConjectureAskedFor) {
    const pud_node_id lemma = h.axiom(h.atom("a"), {h.atom("b")}, 0);
    h.axiom(h.atom("b"), {}, 0);
    h.add_colors();
    h.start_conjecture(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)})}, 1);
    EXPECT_EQ(next_unfold(), (pud_unfold{.leaf = lemma, .goal = 0}));
}

TEST_F(PudSolverIntegrationTest, AChainOfUnitGoalsIsSolvedByForcedUnfoldsAlone) {
    for (int link = 0; link < 20; ++link)
        h.axiom(h.atom("p" + std::to_string(link)), {h.atom("p" + std::to_string(link + 1))}, 0);
    h.axiom(h.atom("p20"), {}, 0);
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.atom("p0")}, 0);
    size_t steps = 0;
    while (h.waiting_solutions[conjecture].empty()) {
        ASSERT_TRUE(h.manifest.unit_unfold_finder_.find_unit_unfold().has_value()) << "step " << steps;
        ASSERT_TRUE(h.advance_solver());
        ++steps;
    }
    EXPECT_THAT(h.enumerate(conjecture, 5, 0), ::testing::ElementsAre(h.atom("q")));
}
