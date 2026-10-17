#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "pud_datalog_oracle.hpp"
#include "pud_solver_manifest_harness.hpp"

namespace {

std::string describe(const std::set<std::vector<int>>& answers) {
    std::string text = "{";
    for (const std::vector<int>& answer : answers) {
        text += "(";
        for (int value : answer)
            text += std::to_string(value) + " ";
        text += ")";
    }
    return text + "}";
}

std::set<std::vector<int>> read_answers(pud_random_solver_manifest_harness& harness, const std::vector<const expr*>& answers) {
    std::set<std::vector<int>> read;
    for (const expr* answer : answers) {
        const std::optional<std::vector<int>> values = datalog_read_answer(harness, answer);
        EXPECT_TRUE(values.has_value()) << "answer is not ground over the domain";
        if (values.has_value())
            read.insert(values.value());
    }
    return read;
}

bool has_duplicates(std::vector<const expr*> answers) {
    std::sort(answers.begin(), answers.end());
    return std::adjacent_find(answers.begin(), answers.end()) != answers.end();
}

}  // namespace

// ---------- flat (non-recursive) programs: the search space is finite ----------

TEST(PudRandomSolverManifestOracleTest, DatalogAnswersMatchABottomUpFixpoint) {
    size_t programs_with_answers = 0;
    size_t total_answers = 0;
    for (unsigned seed = 0; seed < 200; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, false);
        auto harness = std::make_unique<pud_random_solver_manifest_harness>(seed);
        const pud_node_id conjecture = load_datalog(*harness, program);
        const std::vector<const expr*> answers = harness->enumerate(conjecture, 5000, 20000);
        const std::set<std::vector<int>> expected = datalog_answers(program);
        const std::set<std::vector<int>> found = read_answers(*harness, answers);
        EXPECT_EQ(found, expected) << "found " << describe(found) << " expected " << describe(expected);
        programs_with_answers += expected.empty() ? 0 : 1;
        total_answers += expected.size();
    }
    EXPECT_GE(programs_with_answers, 75u) << "the random corpus is too easy to be meaningful";
    EXPECT_GE(total_answers, 150u);
}

TEST(PudRandomSolverManifestOracleTest, EnumerationIsCompleteForFiniteSearchSpaces) {
    for (unsigned seed = 1000; seed < 1200; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, false);
        auto harness = std::make_unique<pud_random_solver_manifest_harness>(seed);
        const pud_node_id conjecture = load_datalog(*harness, program);
        const std::vector<const expr*> answers = harness->enumerate(conjecture, 5000, 20000);
        EXPECT_TRUE(harness->exhausted(conjecture)) << "a finite search space must end exhausted";
        EXPECT_EQ(read_answers(*harness, answers).size(), datalog_answers(program).size());
    }
}

// ---------- recursive programs: answers must be sound even when the search never ends ----------

TEST(PudRandomSolverManifestOracleTest, EveryAnswerIsEntailed) {
    size_t answers_checked = 0;
    for (unsigned seed = 2000; seed < 2200; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, true);
        auto harness = std::make_unique<pud_random_solver_manifest_harness>(seed);
        const pud_node_id conjecture = load_datalog(*harness, program);
        const std::vector<const expr*> answers = harness->enumerate(conjecture, 40, 400);
        const std::set<std::vector<int>> expected = datalog_answers(program);
        for (const std::vector<int>& found : read_answers(*harness, answers))
            EXPECT_TRUE(expected.contains(found)) << "unentailed answer " << describe({found});
        if (harness->exhausted(conjecture))
            EXPECT_EQ(read_answers(*harness, answers), expected) << "an exhausted recursive conjecture must be complete";
        answers_checked += answers.size();
    }
    EXPECT_GE(answers_checked, 150u) << "the random corpus is too easy to be meaningful";
}

// ---------- programs where every answer has exactly one derivation ----------

TEST(PudRandomSolverManifestOracleTest, EnumerationProducesNoDuplicates) {
    {
        pud_random_solver_manifest_harness harness{0};
        harness.add_append();
        const pud_node_id conjecture = harness.start_conjecture(
            harness.functor("q", {harness.var(0), harness.var(1)}),
            {harness.functor("app", {harness.var(0), harness.var(1),
                harness.list({harness.atom("a"), harness.atom("b"), harness.atom("c"), harness.atom("d")})})},
            2);
        const std::vector<const expr*> answers = harness.enumerate(conjecture, 100, 500);
        EXPECT_EQ(answers.size(), 5u);
        EXPECT_FALSE(has_duplicates(answers));
        EXPECT_TRUE(harness.exhausted(conjecture));
    }
    {
        pud_random_solver_manifest_harness harness{0};
        harness.add_member();
        const pud_node_id conjecture = harness.start_conjecture(
            harness.functor("q", {harness.var(0)}),
            {harness.functor("mem", {harness.var(0),
                harness.list({harness.atom("a"), harness.atom("b"), harness.atom("c"), harness.atom("d")})})},
            1);
        const std::vector<const expr*> answers = harness.enumerate(conjecture, 100, 500);
        EXPECT_EQ(answers.size(), 4u);
        EXPECT_FALSE(has_duplicates(answers));
    }
    {
        pud_random_solver_manifest_harness harness{0};
        harness.add_peano_add();
        const pud_node_id conjecture = harness.start_conjecture(
            harness.functor("q", {harness.var(0), harness.var(1)}),
            {harness.functor("add", {harness.var(0), harness.var(1), harness.peano(5)})},
            2);
        const std::vector<const expr*> answers = harness.enumerate(conjecture, 100, 500);
        EXPECT_EQ(answers.size(), 6u);
        EXPECT_FALSE(has_duplicates(answers));
    }
    {
        pud_random_solver_manifest_harness harness{0};
        harness.add_nat();
        const pud_node_id conjecture = harness.start_conjecture(
            harness.functor("q", {harness.var(0)}), {harness.functor("nat", {harness.var(0)})}, 1);
        const std::vector<const expr*> answers = harness.enumerate(conjecture, 25, 500);
        EXPECT_EQ(answers.size(), 25u);
        EXPECT_FALSE(has_duplicates(answers));
    }
}

// ---------- growing the rule trees first (lemmas) must not change any answer ----------

namespace {

// Loads the program like load_datalog, but starts every rule's tree and expands it along its open
// goals before the conjecture starts, so the conjecture also resolves against specialized rule positions.
pud_node_id load_datalog_with_grown_rules(pud_random_solver_manifest_harness& harness, const datalog_program& program, uint32_t depth) {
    for (const datalog_atom& fact : program.facts)
        harness.axiom(datalog_expr(harness, fact), {}, 0);
    std::vector<pud_node_id> rule_roots;
    for (const datalog_rule& rule : program.rules) {
        std::vector<const expr*> body;
        for (const datalog_atom& atom : rule.body)
            body.push_back(datalog_expr(harness, atom));
        rule_roots.push_back(harness.axiom(datalog_expr(harness, rule.head), body, static_cast<uint32_t>(rule.var_count)));
    }
    harness.run_base_descent();
    for (pud_node_id rule_root : rule_roots) {
        if (harness.refuted(rule_root))
            continue;
        harness.grow_along_open_goals(rule_root, depth);
    }
    std::vector<const expr*> answer_args;
    for (int index = 0; index < program.conjecture_var_count; ++index)
        answer_args.push_back(harness.var(index));
    return harness.start_conjecture(
        harness.functor("ans", answer_args),
        {datalog_expr(harness, program.conjecture)},
        static_cast<uint32_t>(program.conjecture_var_count));
}

}  // namespace

TEST(PudRandomSolverManifestOracleTest, GrownRuleTreesLeaveFlatAnswersUnchanged) {
    for (unsigned seed = 3000; seed < 3150; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, false);
        auto harness = std::make_unique<pud_random_solver_manifest_harness>(seed);
        const pud_node_id conjecture = load_datalog_with_grown_rules(*harness, program, 2);
        harness->check_all();
        const std::vector<const expr*> answers = harness->enumerate(conjecture, 5000, 20000);
        EXPECT_EQ(read_answers(*harness, answers), datalog_answers(program));
        EXPECT_TRUE(harness->exhausted(conjecture));
        harness->check_all();
        if (::testing::Test::HasFailure())
            return;
    }
}

TEST(PudRandomSolverManifestOracleTest, GrownRuleTreesKeepRecursiveAnswersSound) {
    for (unsigned seed = 4000; seed < 4150; ++seed) {
        SCOPED_TRACE("seed " + std::to_string(seed));
        const datalog_program program = generate_datalog(seed, true);
        auto harness = std::make_unique<pud_random_solver_manifest_harness>(seed);
        const pud_node_id conjecture = load_datalog_with_grown_rules(*harness, program, 2);
        const std::vector<const expr*> answers = harness->enumerate(conjecture, 40, 400);
        const std::set<std::vector<int>> expected = datalog_answers(program);
        for (const std::vector<int>& found : read_answers(*harness, answers))
            EXPECT_TRUE(expected.contains(found)) << "unentailed answer " << describe({found});
        if (harness->exhausted(conjecture))
            EXPECT_EQ(read_answers(*harness, answers), expected);
        harness->check_all();
        if (::testing::Test::HasFailure())
            return;
    }
}
