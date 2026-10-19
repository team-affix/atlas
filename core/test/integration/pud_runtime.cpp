#include <algorithm>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/pud_runtime.hpp"
#include "pud_datalog_oracle.hpp"
#include "pud_expr_structure.hpp"
#include "value_objects/rule.hpp"

namespace {
std::vector<pud_open_goal> goals_of(pud_runtime& runtime, pud_derivation_id derivation) {
    std::vector<pud_open_goal> goals;
    coroutine<pud_open_goal, void> body_goals = runtime.iterate_body_goals(derivation);
    while (const std::optional<pud_open_goal> goal = body_goals.next())
        goals.push_back(goal.value());
    return goals;
}

struct tracked_frontier {
    std::set<uint32_t> open;
    std::set<uint32_t> proven;
    std::set<uint32_t> seen;
    std::set<uint32_t> under_conjecture;

    size_t apply(const pud_frontier_change& change, bool conjecture) {
        size_t first_seen = 0;
        for (pud_derivation_id derivation : change.opened) {
            open.insert(derivation.value);
            first_seen += note(derivation, conjecture);
        }
        for (pud_derivation_id derivation : change.proven) {
            proven.insert(derivation.value);
            first_seen += note(derivation, conjecture);
        }
        for (pud_derivation_id derivation : change.refuted) {
            open.erase(derivation.value);
            proven.erase(derivation.value);
            first_seen += note(derivation, conjecture);
        }
        return first_seen;
    }

    size_t note(pud_derivation_id derivation, bool conjecture) {
        if (conjecture)
            under_conjecture.insert(derivation.value);
        return seen.insert(derivation.value).second ? 1 : 0;
    }

    pud_frontier_change unfold(pud_runtime& runtime, uint32_t derivation, pud_body_goal_id goal, size_t& first_seen) {
        const bool conjecture = under_conjecture.contains(derivation);
        open.erase(derivation);
        const pud_frontier_change change = runtime.unfold(pud_derivation_id{derivation}, goal);
        first_seen = apply(change, conjecture);
        return change;
    }
};

struct PudRuntimeIntegrationTest : public ::testing::Test {
    expr_pool pool;
    test_functors functors;
    db database;
    pud_runtime runtime{database};

    const expr* var(uint32_t index) { return pool.make_var(index); }

    const expr* functor(const std::string& name, std::vector<const expr*> args) {
        return pool.make_functor(functors.id(name), args);
    }

    const expr* atom(const std::string& name) { return functor(name, {}); }

    void axiom(const expr* head, std::vector<const expr*> body, uint32_t var_count) {
        database.push(rule(head, std::move(body), var_count));
    }

    pud_frontier_change conjecture(const expr* head, std::vector<const expr*> goals, uint32_t var_count) {
        initial_goal_exprs initial_goals;
        for (const expr* goal : goals)
            initial_goals.push(goal);
        return runtime.add_conjecture(head, initial_goals, var_count);
    }

    void add_colors() {
        axiom(functor("color", {atom("red")}), {}, 0);
        axiom(functor("color", {atom("blue")}), {}, 0);
        axiom(functor("warm", {atom("red")}), {}, 0);
    }

    pud_derivation_id open_color_conjecture() {
        add_colors();
        runtime.open_database();
        const pud_frontier_change change = conjecture(
            functor("q", {var(0)}), {functor("color", {var(0)}), functor("warm", {var(0)})}, 1);
        return change.opened.at(0);
    }

    pud_derivation_id add_doomed_p() {
        axiom(functor("t", {atom("a"), atom("b")}), {}, 0);
        axiom(functor("p", {var(0)}), {functor("t", {var(0), var(1)}), functor("t", {var(1), var(0)})}, 2);
        const pud_frontier_change change = runtime.open_database();
        return change.opened.at(0);
    }

    void load(db& target, const datalog_program& program) {
        for (const datalog_atom& fact : program.facts)
            target.push(rule(datalog_expr(*this, fact), {}, 0));
        for (const datalog_rule& datalog_rule : program.rules) {
            std::vector<const expr*> body;
            for (const datalog_atom& body_atom : datalog_rule.body)
                body.push_back(datalog_expr(*this, body_atom));
            target.push(rule(datalog_expr(*this, datalog_rule.head), body, static_cast<uint32_t>(datalog_rule.var_count)));
        }
    }

    pud_frontier_change add_datalog_conjecture(pud_runtime& target, const datalog_program& program) {
        std::vector<const expr*> answer_args;
        for (int index = 0; index < program.conjecture_var_count; ++index)
            answer_args.push_back(var(index));
        initial_goal_exprs goals;
        goals.push(datalog_expr(*this, program.conjecture));
        return target.add_conjecture(
            functor("ans", answer_args), goals, static_cast<uint32_t>(program.conjecture_var_count));
    }

    std::optional<int> read_constant(const expr* arg) {
        for (int constant = 0; constant < k_datalog_constants; ++constant) {
            if (same_structure(arg, atom("k" + std::to_string(constant))))
                return constant;
        }
        return std::nullopt;
    }

    std::optional<std::vector<int>> read_args(const expr* answer) {
        std::vector<int> values;
        for (const expr* arg : std::get<expr::functor>(answer->content).args) {
            const std::optional<int> value = read_constant(arg);
            if (!value.has_value())
                return std::nullopt;
            values.push_back(value.value());
        }
        return values;
    }

    std::optional<datalog_fact> read_fact(const expr* fact) {
        const std::optional<std::vector<int>> args = read_args(fact);
        if (!args.has_value())
            return std::nullopt;
        for (int predicate = 0; predicate < 5; ++predicate) {
            const bool named = std::get<expr::functor>(fact->content).id == functors.id("p" + std::to_string(predicate));
            if (named)
                return datalog_fact{predicate, args.value()};
        }
        return std::nullopt;
    }
};

}

TEST_F(PudRuntimeIntegrationTest, FactsOpenAsProvenDerivations) {
    add_colors();
    const pud_frontier_change change = runtime.open_database();
    EXPECT_TRUE(change.opened.empty());
    EXPECT_EQ(change.proven.size(), 3u);
    EXPECT_TRUE(change.refuted.empty());
    EXPECT_TRUE(change.made_unit.empty());
}

TEST_F(PudRuntimeIntegrationTest, AProvenFactReadsAsItsHead) {
    axiom(functor("color", {atom("red")}), {}, 0);
    const pud_frontier_change change = runtime.open_database();
    ASSERT_EQ(change.proven.size(), 1u);
    EXPECT_TRUE(same_structure(runtime.head_expr(change.proven.at(0)), functor("color", {atom("red")})));
}

TEST_F(PudRuntimeIntegrationTest, ARuleOpensWithItsGoalsAndExactCounts) {
    axiom(functor("nat", {atom("z")}), {}, 0);
    axiom(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1);
    const pud_frontier_change change = runtime.open_database();
    ASSERT_EQ(change.opened.size(), 1u);
    EXPECT_EQ(goals_of(runtime, change.opened.at(0)), (std::vector<pud_open_goal>{{.goal = 0, .candidate_count = 2}}));
    EXPECT_EQ(change.proven.size(), 1u);
}

TEST_F(PudRuntimeIntegrationTest, ARuleWithAnUnprovableGoalOpensRefuted) {
    axiom(functor("r", {var(0)}), {functor("missing", {var(0)})}, 1);
    const pud_frontier_change change = runtime.open_database();
    EXPECT_TRUE(change.opened.empty());
    EXPECT_TRUE(change.proven.empty());
    EXPECT_EQ(change.refuted.size(), 1u);
}

TEST_F(PudRuntimeIntegrationTest, AnEmptyDatabaseOpensNothing) {
    EXPECT_EQ(runtime.open_database(), pud_frontier_change{});
}

TEST_F(PudRuntimeIntegrationTest, OpeningTheDatabaseTwiceIsRejected) {
    runtime.open_database();
    EXPECT_THROW(runtime.open_database(), std::logic_error);
}

TEST_F(PudRuntimeIntegrationTest, AConjectureBeforeTheDatabaseIsOpenIsRejected) {
    add_colors();
    EXPECT_THROW(conjecture(functor("q", {var(0)}), {functor("color", {var(0)})}, 1), std::logic_error);
}

TEST_F(PudRuntimeIntegrationTest, AConjectureOpensWithExactCountsPerGoal) {
    add_colors();
    runtime.open_database();
    const pud_frontier_change change = conjecture(
        functor("q", {var(0)}), {functor("color", {var(0)}), functor("warm", {var(0)})}, 1);
    ASSERT_EQ(change.opened.size(), 1u);
    EXPECT_EQ(goals_of(runtime, change.opened.at(0)),
              (std::vector<pud_open_goal>{{.goal = 0, .candidate_count = 2}, {.goal = 1, .candidate_count = 1}}));
    EXPECT_TRUE(change.proven.empty());
    EXPECT_TRUE(change.refuted.empty());
    EXPECT_TRUE(change.made_unit.empty());
}

TEST_F(PudRuntimeIntegrationTest, AConjectureWithNoGoalsIsProvenAtOnce) {
    add_colors();
    runtime.open_database();
    const pud_frontier_change change = conjecture(functor("q", {atom("red")}), {}, 0);
    EXPECT_TRUE(change.opened.empty());
    ASSERT_EQ(change.proven.size(), 1u);
    EXPECT_TRUE(same_structure(runtime.head_expr(change.proven.at(0)), functor("q", {atom("red")})));
}

TEST_F(PudRuntimeIntegrationTest, AnUnprovableConjectureIsRefutedAtOnce) {
    add_colors();
    runtime.open_database();
    const pud_frontier_change change = conjecture(functor("q", {}), {functor("color", {atom("green")})}, 0);
    EXPECT_TRUE(change.opened.empty());
    EXPECT_TRUE(change.proven.empty());
    EXPECT_EQ(change.refuted.size(), 1u);
}

TEST_F(PudRuntimeIntegrationTest, AConjectureHeadIsNeverACandidate) {
    add_colors();
    runtime.open_database();
    conjecture(functor("q", {var(0)}), {functor("color", {var(0)})}, 1);
    const pud_frontier_change change = conjecture(functor("ans", {var(0)}), {functor("q", {var(0)})}, 1);
    EXPECT_TRUE(change.opened.empty());
    EXPECT_EQ(change.refuted.size(), 1u);
}

TEST_F(PudRuntimeIntegrationTest, UnfoldingReportsOpenedAndRefutedDerivations) {
    const pud_derivation_id derivation = open_color_conjecture();
    const pud_frontier_change change = runtime.unfold(derivation, 0);
    ASSERT_EQ(change.opened.size(), 1u);
    EXPECT_EQ(goals_of(runtime, change.opened.at(0)), (std::vector<pud_open_goal>{{.goal = 1, .candidate_count = 1}}));
    EXPECT_TRUE(change.proven.empty());
    EXPECT_EQ(change.refuted.size(), 1u) << "the blue derivation has nothing warm to prove";
    EXPECT_TRUE(change.made_unit.empty()) << "a new derivation's forced goal is carried by its count";
}

TEST_F(PudRuntimeIntegrationTest, ARefutedDerivationIsNotAlsoReportedOpened) {
    const pud_derivation_id derivation = open_color_conjecture();
    const pud_frontier_change change = runtime.unfold(derivation, 0);
    for (pud_derivation_id opened : change.opened) {
        for (pud_derivation_id refuted : change.refuted)
            EXPECT_NE(opened, refuted);
    }
}

TEST_F(PudRuntimeIntegrationTest, TheUnfoldedDerivationIsNotListed) {
    const pud_derivation_id derivation = open_color_conjecture();
    const pud_frontier_change change = runtime.unfold(derivation, 0);
    for (pud_derivation_id opened : change.opened)
        EXPECT_NE(opened, derivation);
    for (pud_derivation_id listed : change.proven)
        EXPECT_NE(listed, derivation);
    for (pud_derivation_id listed : change.refuted)
        EXPECT_NE(listed, derivation);
}

TEST_F(PudRuntimeIntegrationTest, UnfoldingTheLastGoalProvesTheDerivation) {
    const pud_derivation_id derivation = open_color_conjecture();
    const pud_derivation_id red = runtime.unfold(derivation, 0).opened.at(0);
    const pud_frontier_change change = runtime.unfold(red, 1);
    EXPECT_TRUE(change.opened.empty());
    ASSERT_EQ(change.proven.size(), 1u);
    EXPECT_TRUE(change.refuted.empty());
    EXPECT_TRUE(same_structure(runtime.head_expr(change.proven.at(0)), functor("q", {atom("red")})));
}

TEST_F(PudRuntimeIntegrationTest, AForcedGoalCanBeUnfoldedFirst) {
    const pud_derivation_id derivation = open_color_conjecture();
    const pud_frontier_change change = runtime.unfold(derivation, 1);
    ASSERT_EQ(change.opened.size(), 1u);
    EXPECT_EQ(goals_of(runtime, change.opened.at(0)), (std::vector<pud_open_goal>{{.goal = 0, .candidate_count = 1}}))
        << "only color(red) is left once X is red";
    EXPECT_TRUE(change.refuted.empty());
}

TEST_F(PudRuntimeIntegrationTest, UnfoldingAnAxiomDerivationProvesALemma) {
    axiom(functor("nat", {atom("z")}), {}, 0);
    axiom(functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1);
    const pud_derivation_id derivation = runtime.open_database().opened.at(0);
    const pud_frontier_change change = runtime.unfold(derivation, 0);
    ASSERT_EQ(change.proven.size(), 1u);
    EXPECT_TRUE(same_structure(
        runtime.head_expr(change.proven.at(0)), functor("nat", {functor("s", {atom("z")})})));
    EXPECT_EQ(change.opened.size(), 1u);
}

TEST_F(PudRuntimeIntegrationTest, RefutingACandidateReportsTheExistingGoalItForces) {
    axiom(functor("p", {atom("c")}), {}, 0);
    const pud_derivation_id doomed = add_doomed_p();
    const pud_frontier_change opened = conjecture(functor("ans", {var(0)}), {functor("p", {var(0)})}, 1);
    ASSERT_EQ(opened.opened.size(), 1u);
    const pud_derivation_id waiting = opened.opened.at(0);
    ASSERT_EQ(goals_of(runtime, opened.opened.at(0)), (std::vector<pud_open_goal>{{.goal = 0, .candidate_count = 2}}));

    const pud_frontier_change change = runtime.unfold(doomed, 0);
    EXPECT_EQ(change.made_unit, (std::vector<pud_unit_goal>{{.derivation = waiting, .goal = 0}}));
    const pud_frontier_change forced = runtime.unfold(waiting, 0);
    EXPECT_EQ(forced.opened.size() + forced.proven.size() + forced.refuted.size(), 1u);
}

TEST_F(PudRuntimeIntegrationTest, RefutationKnocksOnToOtherDerivations) {
    const pud_derivation_id doomed = add_doomed_p();
    const pud_frontier_change opened = conjecture(functor("ans", {var(0)}), {functor("p", {var(0)})}, 1);
    ASSERT_EQ(opened.opened.size(), 1u);
    const pud_derivation_id waiting = opened.opened.at(0);

    const pud_frontier_change change = runtime.unfold(doomed, 0);
    EXPECT_NE(std::find(change.refuted.begin(), change.refuted.end(), waiting), change.refuted.end())
        << "the conjecture's only candidate is gone";
    EXPECT_TRUE(change.made_unit.empty());
}

TEST_F(PudRuntimeIntegrationTest, AConsumedDerivationCannotBeUsed) {
    const pud_derivation_id derivation = open_color_conjecture();
    runtime.unfold(derivation, 0);
    EXPECT_THROW(runtime.unfold(derivation, 1), std::logic_error);
    EXPECT_THROW(runtime.head_expr(derivation), std::logic_error);
    EXPECT_THROW(runtime.body_goal_expr(derivation, 1), std::logic_error);
}

TEST_F(PudRuntimeIntegrationTest, ARefutedDerivationCannotBeUsed) {
    const pud_derivation_id derivation = open_color_conjecture();
    const pud_derivation_id refuted = runtime.unfold(derivation, 0).refuted.at(0);
    EXPECT_THROW(runtime.unfold(refuted, 1), std::logic_error);
    EXPECT_THROW(runtime.head_expr(refuted), std::logic_error);
    EXPECT_THROW(runtime.body_goal_expr(refuted, 1), std::logic_error);
}

TEST_F(PudRuntimeIntegrationTest, AForgedDerivationCannotBeUsed) {
    open_color_conjecture();
    const pud_derivation_id forged{9999};
    EXPECT_THROW(runtime.unfold(forged, 0), std::logic_error);
    EXPECT_THROW(runtime.head_expr(forged), std::logic_error);
    EXPECT_THROW(runtime.body_goal_expr(forged, 0), std::logic_error);
}

TEST_F(PudRuntimeIntegrationTest, AProvenDerivationHasNothingToUnfold) {
    const pud_derivation_id derivation = open_color_conjecture();
    const pud_derivation_id red = runtime.unfold(derivation, 0).opened.at(0);
    const pud_derivation_id proven = runtime.unfold(red, 1).proven.at(0);
    EXPECT_THROW(runtime.unfold(proven, 0), std::logic_error);
    EXPECT_THROW(runtime.body_goal_expr(proven, 0), std::logic_error);
    EXPECT_NO_THROW(runtime.head_expr(proven));
}

TEST_F(PudRuntimeIntegrationTest, AGoalThatIsNoLongerOpenCannotBeUsed) {
    const pud_derivation_id derivation = open_color_conjecture();
    const pud_derivation_id red = runtime.unfold(derivation, 1).opened.at(0);
    EXPECT_THROW(runtime.unfold(red, 1), std::logic_error);
    EXPECT_THROW(runtime.body_goal_expr(red, 1), std::logic_error);
    EXPECT_THROW(runtime.unfold(red, 7), std::logic_error);
}

TEST_F(PudRuntimeIntegrationTest, GoalCountsAreReadWhenIterated) {
    axiom(functor("p", {atom("c")}), {}, 0);
    const pud_derivation_id doomed = add_doomed_p();
    const pud_derivation_id waiting = conjecture(functor("ans", {var(0)}), {functor("p", {var(0)})}, 1).opened.at(0);
    EXPECT_EQ(goals_of(runtime, waiting), (std::vector<pud_open_goal>{{.goal = 0, .candidate_count = 2}}));
    runtime.unfold(doomed, 0);
    EXPECT_EQ(goals_of(runtime, waiting), (std::vector<pud_open_goal>{{.goal = 0, .candidate_count = 1}}));
}

TEST_F(PudRuntimeIntegrationTest, NothingIsReadBeforeGoalsAreIterated) {
    open_color_conjecture();
    EXPECT_NO_THROW(runtime.iterate_body_goals(pud_derivation_id{9999}));
}

TEST_F(PudRuntimeIntegrationTest, GoalsOfADerivationThatIsNotOpenCannotBeIterated) {
    const pud_derivation_id derivation = open_color_conjecture();
    const pud_frontier_change change = runtime.unfold(derivation, 0);
    const pud_derivation_id red = change.opened.at(0);
    const pud_derivation_id proven = runtime.unfold(red, 1).proven.at(0);
    EXPECT_THROW(goals_of(runtime, derivation), std::logic_error) << "consumed";
    EXPECT_THROW(goals_of(runtime, change.refuted.at(0)), std::logic_error) << "refuted";
    EXPECT_THROW(goals_of(runtime, proven), std::logic_error) << "proven";
    EXPECT_THROW(goals_of(runtime, pud_derivation_id{9999}), std::logic_error) << "forged";
}

TEST_F(PudRuntimeIntegrationTest, GoalsReadInTheDerivationsBindings) {
    const pud_derivation_id derivation = open_color_conjecture();
    EXPECT_TRUE(same_structure(runtime.body_goal_expr(derivation, 0), functor("color", {var(0)})));
    EXPECT_TRUE(same_structure(runtime.body_goal_expr(derivation, 1), functor("warm", {var(0)})));
    const pud_derivation_id red = runtime.unfold(derivation, 0).opened.at(0);
    EXPECT_TRUE(same_structure(runtime.body_goal_expr(red, 1), functor("warm", {atom("red")})));
}

TEST_F(PudRuntimeIntegrationTest, AnOpenDerivationsHeadReadsInItsBindings) {
    const pud_derivation_id derivation = open_color_conjecture();
    EXPECT_TRUE(same_structure(runtime.head_expr(derivation), functor("q", {var(0)})));
    const pud_derivation_id red = runtime.unfold(derivation, 0).opened.at(0);
    EXPECT_TRUE(same_structure(runtime.head_expr(red), functor("q", {atom("red")})));
}

TEST_F(PudRuntimeIntegrationTest, ExhaustingADatabaseWithoutReadingFindsEveryFactAndAnswer) {
    for (unsigned seed = 0; seed < 12; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, false);
        db seed_database;
        load(seed_database, program);
        pud_runtime seed_runtime{seed_database};

        tracked_frontier frontier;
        frontier.apply(seed_runtime.open_database(), false);
        frontier.apply(add_datalog_conjecture(seed_runtime, program), true);
        size_t unfolds = 0;
        while (!frontier.open.empty() && unfolds < 20000) {
            const uint32_t derivation = *frontier.open.begin();
            const pud_body_goal_id goal = goals_of(seed_runtime, pud_derivation_id{derivation}).front().goal;
            size_t first_seen = 0;
            frontier.unfold(seed_runtime, derivation, goal, first_seen);
            ++unfolds;
        }
        ASSERT_TRUE(frontier.open.empty()) << "not exhausted after " << unfolds << " unfolds";

        std::set<datalog_fact> proven_facts;
        std::set<std::vector<int>> answers;
        for (uint32_t derivation : frontier.proven) {
            const expr* head = seed_runtime.head_expr(pud_derivation_id{derivation});
            if (!frontier.under_conjecture.contains(derivation)) {
                const std::optional<datalog_fact> fact = read_fact(head);
                ASSERT_TRUE(fact.has_value());
                proven_facts.insert(fact.value());
                continue;
            }
            const std::optional<std::vector<int>> answer = read_args(head);
            ASSERT_TRUE(answer.has_value());
            answers.insert(answer.value());
        }
        EXPECT_EQ(proven_facts, datalog_least_model(program));
        EXPECT_EQ(answers, datalog_answers(program));
    }
}

TEST_F(PudRuntimeIntegrationTest, ReportedCountsAreExactWhenReported) {
    size_t unit_checks = 0;
    size_t fresh_checks = 0;
    for (unsigned seed = 0; seed < 12; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, false);
        db seed_database;
        load(seed_database, program);
        pud_runtime seed_runtime{seed_database};

        tracked_frontier frontier;
        pud_frontier_change latest = seed_runtime.open_database();
        frontier.apply(latest, false);
        latest = add_datalog_conjecture(seed_runtime, program);
        frontier.apply(latest, true);
        size_t unfolds = 0;
        while (!frontier.open.empty() && unfolds < 2000) {
            ++unfolds;
            size_t first_seen = 0;
            const bool unit_waiting =
                !latest.made_unit.empty() && frontier.open.contains(latest.made_unit.front().derivation.value);
            if (unit_waiting) {
                const pud_unit_goal unit = latest.made_unit.front();
                latest = frontier.unfold(seed_runtime, unit.derivation.value, unit.goal, first_seen);
                EXPECT_EQ(first_seen, 1u) << "a goal reported unit has exactly one candidate";
                ++unit_checks;
                continue;
            }
            const bool fresh_waiting =
                !latest.opened.empty() && frontier.open.contains(latest.opened.front().value);
            if (fresh_waiting) {
                const uint32_t derivation = latest.opened.front().value;
                const pud_open_goal goal = goals_of(seed_runtime, latest.opened.front()).front();
                latest = frontier.unfold(seed_runtime, derivation, goal.goal, first_seen);
                EXPECT_EQ(first_seen, goal.candidate_count) << "a freshly reported count is exact";
                ++fresh_checks;
                continue;
            }
            const uint32_t derivation = *frontier.open.begin();
            const pud_body_goal_id goal = goals_of(seed_runtime, pud_derivation_id{derivation}).front().goal;
            latest = frontier.unfold(seed_runtime, derivation, goal, first_seen);
        }
    }
    EXPECT_GT(unit_checks, 0u);
    EXPECT_GT(fresh_checks, 0u);
}

TEST_F(PudRuntimeIntegrationTest, TheSameCallsGiveTheSameChanges) {
    for (unsigned seed = 0; seed < 6; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, false);
        db seed_database;
        load(seed_database, program);
        pud_runtime first{seed_database};
        pud_runtime second{seed_database};

        tracked_frontier frontier;
        const pud_frontier_change opened = first.open_database();
        ASSERT_EQ(opened, second.open_database());
        frontier.apply(opened, false);
        const pud_frontier_change conjectured = add_datalog_conjecture(first, program);
        ASSERT_EQ(conjectured, add_datalog_conjecture(second, program));
        frontier.apply(conjectured, true);
        for (size_t unfolds = 0; !frontier.open.empty() && unfolds < 500; ++unfolds) {
            const uint32_t derivation = *frontier.open.rbegin();
            const std::vector<pud_open_goal> goals = goals_of(first, pud_derivation_id{derivation});
            ASSERT_EQ(goals, goals_of(second, pud_derivation_id{derivation}));
            const pud_body_goal_id goal = goals.back().goal;
            const pud_frontier_change expected = second.unfold(pud_derivation_id{derivation}, goal);
            size_t first_seen = 0;
            ASSERT_EQ(frontier.unfold(first, derivation, goal, first_seen), expected);
        }
    }
}
