#include <algorithm>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "functor_fixture.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/pud_random_solver.hpp"
#include "infrastructure/pud_random_expansion_chooser.hpp"
#include "infrastructure/pud_runtime.hpp"
#include "pud_datalog_oracle.hpp"
#include "pud_expr_structure.hpp"
#include "value_objects/rule.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

namespace {
MATCHER_P(SameStructureAs, expected, "") { return same_structure(arg, expected); }

using test_chooser_t = pud_random_expansion_chooser<pud_runtime>;
using test_solver_t = pud_random_solver<pud_runtime, pud_runtime, test_chooser_t>;

struct solver_under_test {
    solver_under_test(uint32_t random_seed)
        : database()
        , runtime(database)
        , rng(random_seed)
        , chooser(runtime, rng)
        , solver(runtime, runtime, chooser)
        , opened(false)
        , solving()
        , answers()
        , ended(false) {}

    db database;
    pud_runtime runtime;
    std::mt19937 rng;
    test_chooser_t chooser;
    test_solver_t solver;
    bool opened;
    std::optional<coroutine<pud_solver_step, void>> solving;
    std::map<size_t, std::vector<const expr*>> answers;
    bool ended;

    void open() {
        if (opened)
            return;
        opened = true;
        runtime.open_database();
    }

    size_t conjecture(const expr* head, std::vector<const expr*> goals, uint32_t var_count) {
        open();
        initial_goal_exprs initial_goals;
        for (const expr* goal : goals)
            initial_goals.push(goal);
        ended = false;
        return solver.add_conjecture(head, initial_goals, var_count);
    }

    bool step() {
        open();
        const bool fresh = !solving.has_value() || solving->done();
        if (fresh)
            solving.emplace(solver.solve());
        solving->resume();
        if (!solving->has_yield()) {
            ended = true;
            return false;
        }
        const pud_solver_step next = solving->consume_yield();
        if (const pud_solution* solution = std::get_if<pud_solution>(&next))
            answers[solution->conjecture].push_back(
                runtime.whnf(solution->derivation).materialize(runtime.head(solution->derivation)));
        return true;
    }

    std::vector<const expr*> enumerate(size_t conjecture, size_t max_answers, size_t max_steps) {
        size_t steps = 0;
        while (answers[conjecture].size() < max_answers && steps < max_steps && step())
            ++steps;
        std::vector<const expr*> found = answers[conjecture];
        if (found.size() > max_answers)
            found.resize(max_answers);
        return found;
    }

    bool run_to_end(size_t max_steps) {
        for (size_t steps = 0; steps < max_steps && !ended; ++steps)
            step();
        return ended;
    }
};

struct PudRandomSolverIntegrationTest : public ::testing::Test {
    expr_pool pool;
    test_functors functors;
    solver_under_test s{0};

    const expr* var(uint32_t index) { return pool.make_var(index); }

    const expr* functor(const std::string& name, std::vector<const expr*> args) {
        return pool.make_functor(functors.id(name), args);
    }

    const expr* atom(const std::string& name) { return functor(name, {}); }

    const expr* q(std::vector<const expr*> args) { return functor("q", std::move(args)); }

    const expr* list(std::vector<const expr*> items) {
        const expr* tail = atom("nil");
        for (auto item = items.rbegin(); item != items.rend(); ++item)
            tail = functor("cons", {*item, tail});
        return tail;
    }

    const expr* peano(uint32_t count) {
        const expr* number = atom("z");
        for (uint32_t step = 0; step < count; ++step)
            number = functor("s", {number});
        return number;
    }

    void axiom(solver_under_test& target, const expr* head, std::vector<const expr*> body, uint32_t var_count) {
        target.database.push(rule(head, std::move(body), var_count));
    }

    void add_colors(solver_under_test& target) {
        axiom(target, functor("color", {atom("red")}), {}, 0);
        axiom(target, functor("color", {atom("blue")}), {}, 0);
        axiom(target, functor("warm", {atom("red")}), {}, 0);
    }

    void add_nat(solver_under_test& target) {
        axiom(target, functor("nat", {atom("z")}), {}, 0);
        axiom(target, functor("nat", {functor("s", {var(0)})}), {functor("nat", {var(0)})}, 1);
    }

    void add_append(solver_under_test& target) {
        axiom(target, functor("app", {atom("nil"), var(0), var(0)}), {}, 1);
        axiom(target,
            functor("app", {functor("cons", {var(0), var(1)}), var(2), functor("cons", {var(0), var(3)})}),
            {functor("app", {var(1), var(2), var(3)})},
            4);
    }

    void add_member(solver_under_test& target) {
        axiom(target, functor("mem", {var(0), functor("cons", {var(0), var(1)})}), {}, 2);
        axiom(target, functor("mem", {var(0), functor("cons", {var(1), var(2)})}), {functor("mem", {var(0), var(2)})}, 3);
    }

    void add_peano_add(solver_under_test& target) {
        axiom(target, functor("add", {atom("z"), var(0), var(0)}), {}, 1);
        axiom(target,
            functor("add", {functor("s", {var(0)}), var(1), functor("s", {var(2)})}),
            {functor("add", {var(0), var(1), var(2)})},
            3);
    }

    void load_axioms(solver_under_test& target, const datalog_program& program) {
        for (const datalog_atom& fact : program.facts)
            axiom(target, datalog_expr(*this, fact), {}, 0);
        for (const datalog_rule& datalog_rule : program.rules) {
            std::vector<const expr*> body;
            for (const datalog_atom& body_atom : datalog_rule.body)
                body.push_back(datalog_expr(*this, body_atom));
            axiom(target, datalog_expr(*this, datalog_rule.head), body, static_cast<uint32_t>(datalog_rule.var_count));
        }
    }

    size_t add_datalog_conjecture(solver_under_test& target, const datalog_program& program) {
        std::vector<const expr*> answer_args;
        for (int index = 0; index < program.conjecture_var_count; ++index)
            answer_args.push_back(var(index));
        return target.conjecture(
            functor("ans", answer_args),
            {datalog_expr(*this, program.conjecture)},
            static_cast<uint32_t>(program.conjecture_var_count));
    }

    void grow_rules(solver_under_test& target, uint32_t depth) {
        target.opened = true;
        const pud_frontier_change opening = target.runtime.open_database();
        std::set<pud_derivation_id> refuted;
        for (pud_derivation_id rule_derivation : opening.opened) {
            std::optional<pud_derivation_id> frontier = rule_derivation;
            for (uint32_t level = 0; level < depth && frontier.has_value(); ++level) {
                if (refuted.contains(frontier.value()))
                    break;
                const pud_body_goal_id goal = target.runtime.body_goals(frontier.value()).front().id;
                const pud_frontier_change change = target.runtime.expand(frontier.value(), goal);
                refuted.insert(change.refuted.begin(), change.refuted.end());
                frontier.reset();
                if (!change.opened.empty())
                    frontier = change.opened.front();
            }
        }
    }

    std::set<std::vector<int>> read_answers(const std::vector<const expr*>& answers) {
        std::set<std::vector<int>> read;
        for (const expr* answer : answers) {
            std::vector<int> values;
            for (const expr* arg : std::get<expr::functor>(answer->content).args) {
                for (int constant = 0; constant < k_datalog_constants; ++constant) {
                    if (same_structure(arg, atom("k" + std::to_string(constant))))
                        values.push_back(constant);
                }
            }
            EXPECT_EQ(values.size(), std::get<expr::functor>(answer->content).args.size())
                << "answer is not ground over the domain";
            read.insert(values);
        }
        return read;
    }

    static std::vector<const expr*> distinct(std::vector<const expr*> answers) {
        std::sort(answers.begin(), answers.end());
        answers.erase(std::unique(answers.begin(), answers.end()), answers.end());
        return answers;
    }

    static bool has_duplicates(std::vector<const expr*> answers) {
        return distinct(answers).size() != answers.size();
    }
};

}

TEST_F(PudRandomSolverIntegrationTest, GroundConjectureAgainstAFactIsSolved) {
    add_colors(s);
    const size_t conjecture = s.conjecture(atom("q"), {functor("color", {atom("red")})}, 0);
    EXPECT_THAT(s.enumerate(conjecture, 10, 50), ElementsAre(SameStructureAs(atom("q"))));
}

TEST_F(PudRandomSolverIntegrationTest, ConjectureVariableIsBoundByEachMatchingFact) {
    add_colors(s);
    const size_t conjecture = s.conjecture(q({var(0)}), {functor("color", {var(0)})}, 1);
    EXPECT_THAT(s.enumerate(conjecture, 10, 50),
        UnorderedElementsAre(SameStructureAs(q({atom("red")})), SameStructureAs(q({atom("blue")}))));
    EXPECT_TRUE(s.run_to_end(10));
}

TEST_F(PudRandomSolverIntegrationTest, ConjectureWithNoMatchingAxiomHasNoAnswers) {
    add_colors(s);
    const size_t conjecture = s.conjecture(atom("q"), {functor("color", {atom("green")})}, 0);
    EXPECT_FALSE(s.step()) << "nothing is left to unfold";
    EXPECT_THAT(s.answers[conjecture], IsEmpty());
}

TEST_F(PudRandomSolverIntegrationTest, ConjectureWithoutGoalsIsSolvedAtOnce) {
    add_colors(s);
    const size_t conjecture = s.conjecture(atom("q"), {}, 0);
    ASSERT_TRUE(s.step());
    EXPECT_THAT(s.answers[conjecture], ElementsAre(SameStructureAs(atom("q"))));
    EXPECT_FALSE(s.step());
}

TEST_F(PudRandomSolverIntegrationTest, ConjunctionKeepsOnlyBindingsThatSatisfyEveryGoal) {
    add_colors(s);
    const size_t conjecture = s.conjecture(q({var(0)}), {functor("color", {var(0)}), functor("warm", {var(0)})}, 1);
    EXPECT_THAT(s.enumerate(conjecture, 10, 50), ElementsAre(SameStructureAs(q({atom("red")}))));
    EXPECT_TRUE(s.run_to_end(10));
}

TEST_F(PudRandomSolverIntegrationTest, ConjunctionWithNoCommonBindingHasNoAnswers) {
    add_colors(s);
    axiom(s, functor("cold", {atom("blue")}), {}, 0);
    const size_t conjecture = s.conjecture(atom("q"), {functor("warm", {var(0)}), functor("cold", {var(0)})}, 1);
    EXPECT_THAT(s.enumerate(conjecture, 10, 50), IsEmpty());
    EXPECT_TRUE(s.run_to_end(10));
}

TEST_F(PudRandomSolverIntegrationTest, ConjectureWhoseOnlyCandidateIsRefutedHasNoAnswers) {
    axiom(s, functor("p", {var(0)}), {functor("missing", {var(0)})}, 1);
    const size_t conjecture = s.conjecture(atom("q"), {functor("p", {atom("a")})}, 0);
    EXPECT_FALSE(s.step());
    EXPECT_THAT(s.answers[conjecture], IsEmpty());
}

TEST_F(PudRandomSolverIntegrationTest, VariablesSharedAcrossGoalsAndRepeatedInTheAnswer) {
    add_colors(s);
    const size_t conjecture = s.conjecture(
        q({var(0), var(0)}), {functor("color", {var(0)}), functor("warm", {var(0)})}, 1);
    EXPECT_THAT(distinct(s.enumerate(conjecture, 10, 100)), ElementsAre(SameStructureAs(q({atom("red"), atom("red")}))));
    EXPECT_TRUE(s.run_to_end(10));
}

TEST_F(PudRandomSolverIntegrationTest, UnconstrainedAnswerVariableStaysAVariable) {
    axiom(s, functor("any", {var(0)}), {}, 1);
    const size_t conjecture = s.conjecture(q({var(0)}), {functor("any", {var(0)})}, 1);
    const std::vector<const expr*> answers = s.enumerate(conjecture, 10, 50);
    ASSERT_EQ(answers.size(), 1u);
    const expr* argument = std::get<expr::functor>(answers.front()->content).args.front();
    EXPECT_TRUE(std::holds_alternative<expr::var>(argument->content));
}

TEST_F(PudRandomSolverIntegrationTest, NaturalNumbersAreEnumeratedInOrder) {
    add_nat(s);
    const size_t conjecture = s.conjecture(q({var(0)}), {functor("nat", {var(0)})}, 1);
    EXPECT_THAT(s.enumerate(conjecture, 5, 100), ElementsAre(
        SameStructureAs(q({peano(0)})), SameStructureAs(q({peano(1)})), SameStructureAs(q({peano(2)})),
        SameStructureAs(q({peano(3)})), SameStructureAs(q({peano(4)}))));
}

TEST_F(PudRandomSolverIntegrationTest, GroundNaturalNumberIsChecked) {
    add_nat(s);
    const size_t conjecture = s.conjecture(atom("q"), {functor("nat", {peano(3)})}, 0);
    EXPECT_THAT(s.enumerate(conjecture, 10, 100), ElementsAre(SameStructureAs(atom("q"))));
    EXPECT_TRUE(s.run_to_end(100));
}

TEST_F(PudRandomSolverIntegrationTest, NonNaturalHasNoAnswersAfterUnfolding) {
    add_nat(s);
    const size_t conjecture = s.conjecture(atom("q"), {functor("nat", {functor("s", {atom("bad")})})}, 0);
    EXPECT_THAT(s.enumerate(conjecture, 10, 100), IsEmpty());
    EXPECT_TRUE(s.run_to_end(100));
}

TEST_F(PudRandomSolverIntegrationTest, AppendBuildsTheConcatenation) {
    add_append(s);
    const size_t conjecture = s.conjecture(
        q({var(0)}), {functor("app", {list({atom("a")}), list({atom("b"), atom("c")}), var(0)})}, 1);
    EXPECT_THAT(s.enumerate(conjecture, 10, 100),
        ElementsAre(SameStructureAs(q({list({atom("a"), atom("b"), atom("c")})}))));
    EXPECT_TRUE(s.run_to_end(100));
}

TEST_F(PudRandomSolverIntegrationTest, AppendSplitsAListEveryWay) {
    add_append(s);
    const size_t conjecture = s.conjecture(
        q({var(0), var(1)}), {functor("app", {var(0), var(1), list({atom("a"), atom("b")})})}, 2);
    EXPECT_THAT(s.enumerate(conjecture, 10, 100), UnorderedElementsAre(
        SameStructureAs(q({list({}), list({atom("a"), atom("b")})})),
        SameStructureAs(q({list({atom("a")}), list({atom("b")})})),
        SameStructureAs(q({list({atom("a"), atom("b")}), list({})}))));
    EXPECT_TRUE(s.run_to_end(100));
}

TEST_F(PudRandomSolverIntegrationTest, MemberFindsEveryElement) {
    add_member(s);
    const size_t conjecture = s.conjecture(q({var(0)}), {functor("mem", {var(0), list({atom("a"), atom("b"), atom("c")})})}, 1);
    EXPECT_THAT(s.enumerate(conjecture, 10, 100), UnorderedElementsAre(
        SameStructureAs(q({atom("a")})), SameStructureAs(q({atom("b")})), SameStructureAs(q({atom("c")}))));
    EXPECT_TRUE(s.run_to_end(100));
}

TEST_F(PudRandomSolverIntegrationTest, PeanoAdditionIsInverted) {
    add_peano_add(s);
    const size_t conjecture = s.conjecture(q({var(0), var(1)}), {functor("add", {var(0), var(1), peano(2)})}, 2);
    EXPECT_THAT(s.enumerate(conjecture, 10, 100), UnorderedElementsAre(
        SameStructureAs(q({peano(0), peano(2)})),
        SameStructureAs(q({peano(1), peano(1)})),
        SameStructureAs(q({peano(2), peano(0)}))));
    EXPECT_TRUE(s.run_to_end(100));
}

TEST_F(PudRandomSolverIntegrationTest, TransitiveClosureTerminatesOnAFiniteGraph) {
    axiom(s, functor("parent", {atom("a"), atom("b")}), {}, 0);
    axiom(s, functor("parent", {atom("b"), atom("c")}), {}, 0);
    axiom(s, functor("anc", {var(0), var(1)}), {functor("parent", {var(0), var(1)})}, 2);
    axiom(s, functor("anc", {var(0), var(1)}), {functor("parent", {var(0), var(2)}), functor("anc", {var(2), var(1)})}, 3);
    const size_t conjecture = s.conjecture(q({var(0)}), {functor("anc", {atom("a"), var(0)})}, 1);
    EXPECT_THAT(s.enumerate(conjecture, 10, 200),
        UnorderedElementsAre(SameStructureAs(q({atom("b")})), SameStructureAs(q({atom("c")}))));
    EXPECT_TRUE(s.run_to_end(200));
}

TEST_F(PudRandomSolverIntegrationTest, SharedVariablesAcrossGoalsAreRespected) {
    add_append(s);
    add_member(s);
    const size_t conjecture = s.conjecture(q({var(0)}), {
        functor("mem", {var(0), list({atom("a"), atom("b"), atom("c")})}),
        functor("app", {list({atom("x")}), list({var(0)}), list({atom("x"), atom("b")})})}, 1);
    EXPECT_THAT(s.enumerate(conjecture, 10, 200), ElementsAre(SameStructureAs(q({atom("b")}))));
    EXPECT_TRUE(s.run_to_end(200));
}

TEST_F(PudRandomSolverIntegrationTest, RuleWithoutABaseNeverSolvesButNeverEnds) {
    axiom(s, atom("p"), {atom("p")}, 0);
    const size_t conjecture = s.conjecture(atom("q"), {atom("p")}, 0);
    EXPECT_THAT(s.enumerate(conjecture, 1, 50), IsEmpty());
    EXPECT_FALSE(s.ended);
}

TEST_F(PudRandomSolverIntegrationTest, ManyNaturalNumbersStayCorrect) {
    add_nat(s);
    const size_t conjecture = s.conjecture(q({var(0)}), {functor("nat", {var(0)})}, 1);
    const std::vector<const expr*> answers = s.enumerate(conjecture, 12, 500);
    ASSERT_EQ(answers.size(), 12u);
    for (uint32_t number = 0; number < 12; ++number)
        EXPECT_THAT(answers, ::testing::Contains(SameStructureAs(q({peano(number)})))) << number;
}

TEST_F(PudRandomSolverIntegrationTest, TwoConjecturesShareTheDatabase) {
    add_colors(s);
    const size_t warm = s.conjecture(functor("w", {var(0)}), {functor("warm", {var(0)})}, 1);
    const size_t color = s.conjecture(functor("c", {var(0)}), {functor("color", {var(0)})}, 1);
    EXPECT_TRUE(s.run_to_end(100));
    EXPECT_THAT(s.answers[warm], ElementsAre(SameStructureAs(functor("w", {atom("red")}))));
    EXPECT_THAT(s.answers[color], UnorderedElementsAre(
        SameStructureAs(functor("c", {atom("red")})), SameStructureAs(functor("c", {atom("blue")}))));
}

TEST_F(PudRandomSolverIntegrationTest, FiniteConjectureCompletesWhileAnInfiniteOneContinues) {
    add_colors(s);
    add_nat(s);
    const size_t colors = s.conjecture(q({var(0)}), {functor("color", {var(0)})}, 1);
    const size_t numbers = s.conjecture(functor("n", {var(0)}), {functor("nat", {var(0)})}, 1);
    EXPECT_THAT(distinct(s.enumerate(colors, 10, 200)),
        UnorderedElementsAre(SameStructureAs(q({atom("red")})), SameStructureAs(q({atom("blue")}))));
    EXPECT_EQ(s.enumerate(numbers, 5, 200).size(), 5u);
    EXPECT_FALSE(s.ended);
}

TEST_F(PudRandomSolverIntegrationTest, ConjectureAddedAfterSolvingEndedIsStillSolved) {
    add_colors(s);
    const size_t first = s.conjecture(q({var(0)}), {functor("warm", {var(0)})}, 1);
    EXPECT_TRUE(s.run_to_end(100));
    EXPECT_THAT(s.answers[first], ElementsAre(SameStructureAs(q({atom("red")}))));
    const size_t second = s.conjecture(q({var(0)}), {functor("color", {var(0)})}, 1);
    EXPECT_TRUE(s.run_to_end(100));
    EXPECT_THAT(distinct(s.answers[second]),
        UnorderedElementsAre(SameStructureAs(q({atom("red")})), SameStructureAs(q({atom("blue")}))));
}

TEST_F(PudRandomSolverIntegrationTest, ConjectureShapedLikeAnotherConjecturesHeadIsNeverSolvedByIt) {
    add_colors(s);
    s.conjecture(functor("pair", {var(0), var(0)}), {functor("color", {var(0)})}, 1);
    const size_t user = s.conjecture(atom("u"), {functor("pair", {atom("red"), atom("red")})}, 0);
    EXPECT_TRUE(s.run_to_end(100));
    EXPECT_THAT(s.answers[user], IsEmpty());
}

TEST_F(PudRandomSolverIntegrationTest, SameSeedSolvesTheSameWay) {
    solver_under_test again{0};
    for (solver_under_test* each : {&s, &again}) {
        add_nat(*each);
        add_append(*each);
        each->conjecture(q({var(0)}), {functor("nat", {var(0)})}, 1);
        each->conjecture(q({var(0), var(1)}), {functor("app", {var(0), var(1), list({atom("a"), atom("b")})})}, 2);
    }
    for (int step = 0; step < 60; ++step) {
        ASSERT_EQ(s.step(), again.step());
        ASSERT_EQ(s.answers.size(), again.answers.size());
    }
    for (const auto& [conjecture, answers] : s.answers) {
        ASSERT_EQ(answers.size(), again.answers.at(conjecture).size());
        for (size_t index = 0; index < answers.size(); ++index)
            EXPECT_TRUE(same_structure(answers.at(index), again.answers.at(conjecture).at(index)));
    }
}

TEST_F(PudRandomSolverIntegrationTest, EnumerationProducesNoDuplicates) {
    {
        solver_under_test each{0};
        add_append(each);
        const size_t conjecture = each.conjecture(q({var(0), var(1)}),
            {functor("app", {var(0), var(1), list({atom("a"), atom("b"), atom("c"), atom("d")})})}, 2);
        const std::vector<const expr*> answers = each.enumerate(conjecture, 100, 500);
        EXPECT_EQ(answers.size(), 5u);
        EXPECT_FALSE(has_duplicates(answers));
        EXPECT_TRUE(each.run_to_end(500));
    }
    {
        solver_under_test each{0};
        add_member(each);
        const size_t conjecture = each.conjecture(q({var(0)}),
            {functor("mem", {var(0), list({atom("a"), atom("b"), atom("c"), atom("d")})})}, 1);
        const std::vector<const expr*> answers = each.enumerate(conjecture, 100, 500);
        EXPECT_EQ(answers.size(), 4u);
        EXPECT_FALSE(has_duplicates(answers));
    }
    {
        solver_under_test each{0};
        add_peano_add(each);
        const size_t conjecture = each.conjecture(q({var(0), var(1)}), {functor("add", {var(0), var(1), peano(5)})}, 2);
        const std::vector<const expr*> answers = each.enumerate(conjecture, 100, 500);
        EXPECT_EQ(answers.size(), 6u);
        EXPECT_FALSE(has_duplicates(answers));
    }
    {
        solver_under_test each{0};
        add_nat(each);
        const size_t conjecture = each.conjecture(q({var(0)}), {functor("nat", {var(0)})}, 1);
        const std::vector<const expr*> answers = each.enumerate(conjecture, 25, 500);
        EXPECT_EQ(answers.size(), 25u);
        EXPECT_FALSE(has_duplicates(answers));
    }
}

TEST_F(PudRandomSolverIntegrationTest, DatalogAnswersMatchABottomUpFixpoint) {
    size_t programs_with_answers = 0;
    size_t total_answers = 0;
    for (unsigned seed = 0; seed < 200; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, false);
        solver_under_test seeded{seed};
        load_axioms(seeded, program);
        const size_t conjecture = add_datalog_conjecture(seeded, program);
        ASSERT_TRUE(seeded.run_to_end(20000)) << "a finite search space must end";
        const std::set<std::vector<int>> expected = datalog_answers(program);
        EXPECT_EQ(read_answers(seeded.answers[conjecture]), expected);
        programs_with_answers += expected.empty() ? 0 : 1;
        total_answers += expected.size();
    }
    EXPECT_GE(programs_with_answers, 75u) << "the random corpus is too easy to be meaningful";
    EXPECT_GE(total_answers, 150u);
}

TEST_F(PudRandomSolverIntegrationTest, EnumerationIsCompleteForFiniteSearchSpaces) {
    for (unsigned seed = 1000; seed < 1200; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, false);
        solver_under_test seeded{seed};
        load_axioms(seeded, program);
        const size_t conjecture = add_datalog_conjecture(seeded, program);
        EXPECT_TRUE(seeded.run_to_end(20000)) << "a finite search space must end";
        EXPECT_EQ(read_answers(seeded.answers[conjecture]).size(), datalog_answers(program).size());
    }
}

TEST_F(PudRandomSolverIntegrationTest, EveryAnswerOfARecursiveProgramIsEntailed) {
    size_t answers_checked = 0;
    for (unsigned seed = 2000; seed < 2200; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, true);
        solver_under_test seeded{seed};
        load_axioms(seeded, program);
        const size_t conjecture = add_datalog_conjecture(seeded, program);
        const std::vector<const expr*> answers = seeded.enumerate(conjecture, 40, 400);
        const std::set<std::vector<int>> expected = datalog_answers(program);
        for (const std::vector<int>& found : read_answers(answers))
            EXPECT_TRUE(expected.contains(found));
        if (seeded.ended)
            EXPECT_EQ(read_answers(answers), expected) << "a recursive conjecture that ended must be complete";
        answers_checked += answers.size();
    }
    EXPECT_GE(answers_checked, 150u) << "the random corpus is too easy to be meaningful";
}

TEST_F(PudRandomSolverIntegrationTest, GrownRuleDerivationsLeaveFlatAnswersUnchanged) {
    for (unsigned seed = 3000; seed < 3150; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, false);
        solver_under_test seeded{seed};
        load_axioms(seeded, program);
        grow_rules(seeded, 2);
        const size_t conjecture = add_datalog_conjecture(seeded, program);
        EXPECT_TRUE(seeded.run_to_end(20000));
        EXPECT_EQ(read_answers(seeded.answers[conjecture]), datalog_answers(program));
    }
}

TEST_F(PudRandomSolverIntegrationTest, GrownRuleDerivationsKeepRecursiveAnswersSound) {
    for (unsigned seed = 4000; seed < 4150; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, true);
        solver_under_test seeded{seed};
        load_axioms(seeded, program);
        grow_rules(seeded, 2);
        const size_t conjecture = add_datalog_conjecture(seeded, program);
        const std::vector<const expr*> answers = seeded.enumerate(conjecture, 40, 400);
        const std::set<std::vector<int>> expected = datalog_answers(program);
        for (const std::vector<int>& found : read_answers(answers))
            EXPECT_TRUE(expected.contains(found));
        if (seeded.ended)
            EXPECT_EQ(read_answers(answers), expected);
    }
}
