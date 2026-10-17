#include <algorithm>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_manifest_harness.hpp"

using ::testing::ElementsAre;
using ::testing::UnorderedElementsAre;

struct PudRandomUnitSolverManifestTest : public ::testing::Test {
    pud_random_unit_solver_manifest_harness h{0};

    const expr* q(std::vector<const expr*> args) { return h.functor("q", std::move(args)); }

    std::vector<const expr*> distinct(std::vector<const expr*> answers) {
        std::sort(answers.begin(), answers.end());
        answers.erase(std::unique(answers.begin(), answers.end()), answers.end());
        return answers;
    }
};

// ---------- unit unfolds dominate decisions ----------

TEST_F(PudRandomUnitSolverManifestTest, UnitGoalIsUnfoldedBeforeAnyDecision) {
    for (uint32_t random_seed = 0; random_seed < 8; ++random_seed) {
        pud_random_unit_solver_manifest_harness seeded{random_seed};
        seeded.add_colors();
        const pud_node_id conjecture = seeded.start_conjecture(
            q({seeded.var(0)}), {seeded.functor("color", {seeded.var(0)}), seeded.functor("warm", {seeded.var(0)})}, 1);
        coroutine<pud_unfold, void>& unfolding = seeded.unfold_loop();
        unfolding.resume();
        ASSERT_TRUE(unfolding.has_yield());
        EXPECT_EQ(unfolding.consume_yield(), (pud_unfold{conjecture, 1})) << "seed " << random_seed;
    }
}

TEST_F(PudRandomUnitSolverManifestTest, UnitQueryOfAConjectureIsQueued) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(h.atom("q"), {h.functor("warm", {h.var(0)})}, 1);
    ASSERT_FALSE(h.manifest.unit_queries_.empty());
    EXPECT_EQ(h.manifest.unit_queries_.front(), h.query_of(conjecture, 0).value());
}

TEST_F(PudRandomUnitSolverManifestTest, ForcedUnfoldsReachAxiomLeavesOldestFirst) {
    h.add_colors();
    const pud_node_id warmcolor = h.axiom(
        h.functor("warmcolor", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    const pud_node_id conjecture = h.start_conjecture(q({h.var(0)}), {h.functor("warmcolor", {h.var(0)})}, 1);
    coroutine<pud_unfold, void>& unfolding = h.unfold_loop();
    unfolding.resume();
    ASSERT_TRUE(unfolding.has_yield());
    EXPECT_EQ(unfolding.consume_yield(), (pud_unfold{warmcolor, 1})) << "the axiom's unit query was queued first";
    EXPECT_THAT(distinct(h.enumerate(conjecture, 10, 50)), ElementsAre(q({h.atom("red")})));
    h.check_all();
}

TEST_F(PudRandomUnitSolverManifestTest, ChainOfUnitGoalsIsSolvedWithoutDecisions) {
    h.add_graph();
    const pud_node_id conjecture = h.start_conjecture(
        h.atom("q"), {h.functor("parent", {h.atom("a"), h.var(0)}), h.functor("parent", {h.var(0), h.atom("c")})}, 1);
    EXPECT_TRUE(h.next_solution(conjecture, 10).has_value());
    h.check_all();
}

TEST_F(PudRandomUnitSolverManifestTest, UnitUnfoldsKeepAnswersComplete) {
    h.add_colors();
    const pud_node_id conjecture = h.start_conjecture(
        q({h.var(0), h.var(1)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(1)})}, 2);
    EXPECT_THAT(distinct(h.enumerate(conjecture, 10, 50)),
        UnorderedElementsAre(q({h.atom("red"), h.atom("red")}), q({h.atom("blue"), h.atom("red")})));
    EXPECT_TRUE(h.exhausted(conjecture));
    h.check_all();
}

// ---------- invariants on well-behaved programs ----------

TEST_F(PudRandomUnitSolverManifestTest, WellBehavedProgramsKeepEveryInvariant) {
    for (uint32_t random_seed = 0; random_seed < 4; ++random_seed) {
        for (int program = 0; program < 5; ++program) {
            SCOPED_TRACE("seed " + std::to_string(random_seed) + " program " + std::to_string(program));
            pud_random_unit_solver_manifest_harness seeded{random_seed};
            pud_node_id conjecture = 0;
            if (program == 0) {
                seeded.add_colors();
                conjecture = seeded.start_conjecture(seeded.functor("q", {seeded.var(0)}),
                    {seeded.functor("color", {seeded.var(0)}), seeded.functor("warm", {seeded.var(0)})}, 1);
            } else if (program == 1) {
                seeded.add_nat();
                conjecture = seeded.start_conjecture(seeded.functor("q", {seeded.var(0)}), {seeded.functor("nat", {seeded.var(0)})}, 1);
            } else if (program == 2) {
                seeded.add_append();
                conjecture = seeded.start_conjecture(seeded.functor("q", {seeded.var(0), seeded.var(1)}),
                    {seeded.functor("app", {seeded.var(0), seeded.var(1), seeded.list({seeded.atom("a"), seeded.atom("b")})})}, 2);
            } else if (program == 3) {
                seeded.add_member();
                conjecture = seeded.start_conjecture(seeded.functor("q", {seeded.var(0)}),
                    {seeded.functor("mem", {seeded.var(0), seeded.list({seeded.atom("a"), seeded.atom("b")})})}, 1);
            } else {
                seeded.add_graph();
                conjecture = seeded.start_conjecture(seeded.functor("q", {seeded.var(0)}),
                    {seeded.functor("anc", {seeded.atom("a"), seeded.var(0)})}, 1);
            }
            seeded.check_all();
            std::vector<const expr*> answers;
            for (int step = 0; step < 30 && seeded.step(conjecture, answers); ++step)
                seeded.check_all();
        }
    }
}

TEST_F(PudRandomUnitSolverManifestTest, AnswersMatchTheDecisionOnlyManifest) {
    pud_random_solver_manifest_harness plain{0};
    h.add_graph();
    plain.add_graph();
    const pud_node_id forced = h.start_conjecture(q({h.var(0)}), {h.functor("anc", {h.atom("a"), h.var(0)})}, 1);
    const pud_node_id decided = plain.start_conjecture(
        plain.functor("q", {plain.var(0)}), {plain.functor("anc", {plain.atom("a"), plain.var(0)})}, 1);
    EXPECT_THAT(distinct(h.enumerate(forced, 10, 200)), UnorderedElementsAre(q({h.atom("b")}), q({h.atom("c")})));
    std::vector<const expr*> decided_answers = plain.enumerate(decided, 10, 200);
    std::sort(decided_answers.begin(), decided_answers.end());
    decided_answers.erase(std::unique(decided_answers.begin(), decided_answers.end()), decided_answers.end());
    EXPECT_THAT(decided_answers, UnorderedElementsAre(
        plain.functor("q", {plain.atom("b")}), plain.functor("q", {plain.atom("c")})));
    EXPECT_TRUE(h.exhausted(forced));
    EXPECT_TRUE(plain.exhausted(decided));
}
