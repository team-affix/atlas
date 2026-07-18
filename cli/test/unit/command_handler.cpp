// command_handler: parameterized tests for basic_command_handler, ridge_command_handler,
// ridge_fc_command_handler, horizon_command_handler, genius_command_handler, and dbuct_*.
// Example DBs under cli/examples/.

#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/basic_command_handler.hpp"
#include "infrastructure/dbuct_genius_command_handler.hpp"
#include "infrastructure/dbuct_horizon_command_handler.hpp"
#include "infrastructure/dbuct_ridge_command_handler.hpp"
#include "infrastructure/dbuct_ridge_fc_command_handler.hpp"
#include "infrastructure/genius_command_handler.hpp"
#include "infrastructure/horizon_command_handler.hpp"
#include "infrastructure/ridge_command_handler.hpp"
#include "infrastructure/ridge_fc_command_handler.hpp"

using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::Not;

enum class cli_solver_kind {
    basic, ridge, ridge_fc, horizon, genius, dbuct_ridge, dbuct_ridge_fc, dbuct_horizon, dbuct_genius
};

struct CommandHandlerParamTest : public ::testing::TestWithParam<cli_solver_kind> {
    static constexpr size_t kMaxResolutions       = 1000;
    static constexpr uint32_t kSeed               = 0;
    static constexpr double kExplorationConstant  = 1.414;
};

INSTANTIATE_TEST_SUITE_P(
    AllSolvers,
    CommandHandlerParamTest,
    ::testing::Values(cli_solver_kind::basic, cli_solver_kind::ridge, cli_solver_kind::ridge_fc,
                      cli_solver_kind::horizon, cli_solver_kind::genius, cli_solver_kind::dbuct_ridge,
                      cli_solver_kind::dbuct_ridge_fc, cli_solver_kind::dbuct_horizon,
                      cli_solver_kind::dbuct_genius),
    [](const auto& info) {
        switch (info.param) {
            case cli_solver_kind::basic:           return "basic";
            case cli_solver_kind::ridge:           return "ridge";
            case cli_solver_kind::ridge_fc:        return "ridge_fc";
            case cli_solver_kind::horizon:         return "horizon";
            case cli_solver_kind::genius:          return "genius";
            case cli_solver_kind::dbuct_ridge:     return "dbuct_ridge";
            case cli_solver_kind::dbuct_ridge_fc:  return "dbuct_ridge_fc";
            case cli_solver_kind::dbuct_horizon:   return "dbuct_horizon";
            case cli_solver_kind::dbuct_genius:    return "dbuct_genius";
        }
        return "unknown";
    });

namespace {

constexpr size_t kEnterLinesPerSolution = 1;

constexpr const char kBindingXEqA[] = "  X = a\n";
constexpr const char kBindingXEqB[] = "  X = b\n";
constexpr const char kBindingXEqC[] = "  X = c\n";
constexpr const char kBindingXEqBob[] = "  X = bob\n";
constexpr const char kBindingXEqLiz[] = "  X = liz\n";
constexpr const char kBindingXEqPat[] = "  X = pat\n";
constexpr const char kBindingXEqJim[] = "  X = jim\n";
constexpr const char kBindingXEqAnn[] = "  X = ann\n";
constexpr const char kBindingXEqTrue[] = "  X = true\n";
constexpr const char kBindingXEqFalse[] = "  X = false\n";
constexpr const char kBindingYEqTrue[] = "  Y = true\n";
constexpr const char kBindingXEqOne[] = "  X = 1\n";
constexpr const char kBindingXEqTwo[] = "  X = 2\n";
constexpr const char kBindingYEqTwo[] = "  Y = 2\n";
constexpr const char kBindingYEqThree[] = "  Y = 3\n";
constexpr const char kBindingXEqEmptyList[] = "  X = []\n";
constexpr const char kBindingXEqBinEmpty[] = "  X = bin([], [])\n";

size_t count_substr(const std::string& haystack, const std::string& needle) {
    size_t count = 0;
    size_t pos = 0;
    while ((pos = haystack.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

std::string stdin_for_solutions(size_t solution_count) {
    return std::string(solution_count * kEnterLinesPerSolution, '\n');
}

void construct_handler(const std::string& file, const std::string& goal, size_t max_res,
                       cli_solver_kind kind) {
    switch (kind) {
        case cli_solver_kind::basic:
            basic_command_handler(file, goal, max_res, CommandHandlerParamTest::kSeed);
            break;
        case cli_solver_kind::ridge:
            ridge_command_handler(
                file, goal, max_res, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant);
            break;
        case cli_solver_kind::ridge_fc:
            ridge_fc_command_handler(
                file, goal, max_res, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant);
            break;
        case cli_solver_kind::horizon:
            horizon_command_handler(
                file, goal, max_res, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant);
            break;
        case cli_solver_kind::genius:
            genius_command_handler(
                file, goal, max_res, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant,
                CommandHandlerParamTest::kExplorationConstant);
            break;
        case cli_solver_kind::dbuct_ridge:
            dbuct_ridge_command_handler(
                file, goal, max_res, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant);
            break;
        case cli_solver_kind::dbuct_ridge_fc:
            dbuct_ridge_fc_command_handler(
                file, goal, max_res, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant);
            break;
        case cli_solver_kind::dbuct_horizon:
            dbuct_horizon_command_handler(
                file, goal, max_res, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant);
            break;
        case cli_solver_kind::dbuct_genius:
            dbuct_genius_command_handler(
                file, goal, max_res, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant,
                CommandHandlerParamTest::kExplorationConstant);
            break;
    }
}

std::string run_handler_capture(
    const std::string& file,
    const std::string& goal,
    size_t max_resolutions,
    const std::string& stdin_lines,
    cli_solver_kind kind) {
    std::ostringstream captured;
    std::istringstream fake_in(stdin_lines);
    std::streambuf* old_out = std::cout.rdbuf(captured.rdbuf());
    std::streambuf* old_in  = std::cin.rdbuf(fake_in.rdbuf());
    switch (kind) {
        case cli_solver_kind::basic:
            basic_command_handler(file, goal, max_resolutions, CommandHandlerParamTest::kSeed)();
            break;
        case cli_solver_kind::ridge:
            ridge_command_handler(
                file, goal, max_resolutions, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant)();
            break;
        case cli_solver_kind::ridge_fc:
            ridge_fc_command_handler(
                file, goal, max_resolutions, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant)();
            break;
        case cli_solver_kind::horizon:
            horizon_command_handler(
                file, goal, max_resolutions, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant)();
            break;
        case cli_solver_kind::genius:
            genius_command_handler(
                file, goal, max_resolutions, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant,
                CommandHandlerParamTest::kExplorationConstant)();
            break;
        case cli_solver_kind::dbuct_ridge:
            dbuct_ridge_command_handler(
                file, goal, max_resolutions, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant)();
            break;
        case cli_solver_kind::dbuct_ridge_fc:
            dbuct_ridge_fc_command_handler(
                file, goal, max_resolutions, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant)();
            break;
        case cli_solver_kind::dbuct_horizon:
            dbuct_horizon_command_handler(
                file, goal, max_resolutions, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant)();
            break;
        case cli_solver_kind::dbuct_genius:
            dbuct_genius_command_handler(
                file, goal, max_resolutions, CommandHandlerParamTest::kSeed,
                CommandHandlerParamTest::kExplorationConstant,
                CommandHandlerParamTest::kExplorationConstant)();
            break;
    }
    std::cout.rdbuf(old_out);
    std::cin.rdbuf(old_in);
    return captured.str();
}

}  // namespace

// ---------------------------------------------------------------------------
// Construction (parse + session emplace must not throw)
// ---------------------------------------------------------------------------

TEST_P(CommandHandlerParamTest, ConstructsFromEqExample) {
    EXPECT_NO_THROW(construct_handler("cli/examples/eq/db.chc", "eq(a, a)", kMaxResolutions, GetParam()));
}

TEST_P(CommandHandlerParamTest, ConstructsFromAncestorExample) {
    EXPECT_NO_THROW(construct_handler(
        "cli/examples/ancestor/db.chc", "ancestor(tom, bob)", kMaxResolutions, GetParam()));
}

TEST_P(CommandHandlerParamTest, ConstructsFromReachabilityExample) {
    EXPECT_NO_THROW(construct_handler(
        "cli/examples/reachability/db.chc", "reach(0, X)", kMaxResolutions, GetParam()));
}

TEST_P(CommandHandlerParamTest, ConstructsFromMemberExample) {
    EXPECT_NO_THROW(construct_handler(
        "cli/examples/member/db.chc", "member(X, [a, b, c])", kMaxResolutions, GetParam()));
}

TEST_P(CommandHandlerParamTest, ConstructsFromSatExample) {
    EXPECT_NO_THROW(construct_handler(
        "cli/examples/sat/db.chc", "or(true, X, Y)", kMaxResolutions, GetParam()));
}

TEST_P(CommandHandlerParamTest, ConstructsFromTreesExample) {
    EXPECT_NO_THROW(construct_handler(
        "cli/examples/trees/db.chc", "in(X, bin([], []))", kMaxResolutions, GetParam()));
}

TEST_P(CommandHandlerParamTest, ConstructsFromArithmeticExample) {
    EXPECT_NO_THROW(construct_handler(
        "cli/examples/arithmetic/db.chc", "even(zero)", kMaxResolutions, GetParam()));
}

TEST_P(CommandHandlerParamTest, ConstructsFromExprExample) {
    EXPECT_NO_THROW(construct_handler(
        "cli/examples/expr/db.chc", "eval(E, zero)", kMaxResolutions, GetParam()));
}

TEST_P(CommandHandlerParamTest, BadFileThrowsOnConstruction) {
    EXPECT_THROW(
        construct_handler("cli/examples/nonexistent/db.chc", "p(X)", kMaxResolutions, GetParam()),
        std::runtime_error);
}

TEST_P(CommandHandlerParamTest, BadGoalThrowsOnConstruction) {
    EXPECT_THROW(
        construct_handler("cli/examples/eq/db.chc", ":-", kMaxResolutions, GetParam()),
        std::runtime_error);
}

// ---------------------------------------------------------------------------
// operator() — solve loop via stdout
// ---------------------------------------------------------------------------

TEST_P(CommandHandlerParamTest, UnsatisfiableGoalRefutesWithoutSolvableTick) {
    static constexpr size_t kBudget = 64;
    const std::string out = run_handler_capture(
        "cli/examples/eq/db.chc", "eq(a, b)", kBudget, stdin_for_solutions(0), GetParam());
    EXPECT_THAT(out, HasSubstr("REFUTED"));
    EXPECT_THAT(out, Not(HasSubstr("SOLVED")));
}

TEST_P(CommandHandlerParamTest, EqGroundSolveThenRefute) {
    const std::string out = run_handler_capture(
        "cli/examples/eq/db.chc", "eq(a, a)", kMaxResolutions, stdin_for_solutions(1), GetParam());
    EXPECT_THAT(out, HasSubstr("SOLVED"));
    EXPECT_THAT(out, HasSubstr("REFUTED"));
    EXPECT_EQ(count_substr(out, "SOLVED"), 1u);
}

TEST_P(CommandHandlerParamTest, ArithmeticGroundGoalSolveThenRefute) {
    const std::string out = run_handler_capture(
        "cli/examples/arithmetic/db.chc", "even(zero)", kMaxResolutions, stdin_for_solutions(1),
        GetParam());
    EXPECT_THAT(out, HasSubstr("SOLVED"));
    EXPECT_THAT(out, HasSubstr("REFUTED"));
    EXPECT_EQ(count_substr(out, "SOLVED"), 1u);
}

TEST_P(CommandHandlerParamTest, TreesGoalEnumeratesTreeMembership) {
    const std::string out = run_handler_capture(
        "cli/examples/trees/db.chc", "in(X, bin([], []))", kMaxResolutions, stdin_for_solutions(5),
        GetParam());
    EXPECT_GE(count_substr(out, "SOLVED"), 2u);
    EXPECT_GE(count_substr(out, kBindingXEqEmptyList), 1u);
    EXPECT_GE(count_substr(out, kBindingXEqBinEmpty), 1u);
    EXPECT_THAT(out, HasSubstr("REFUTED"));
}

TEST_P(CommandHandlerParamTest, AncestorGoalPrintsVarBinding) {
    const std::string out = run_handler_capture(
        "cli/examples/ancestor/db.chc", "ancestor(tom, X)", kMaxResolutions, stdin_for_solutions(6),
        GetParam());
    EXPECT_EQ(count_substr(out, "SOLVED"), 5u);
    EXPECT_EQ(count_substr(out, kBindingXEqBob), 1u);
    EXPECT_EQ(count_substr(out, kBindingXEqLiz), 1u);
    EXPECT_EQ(count_substr(out, kBindingXEqPat), 1u);
    EXPECT_EQ(count_substr(out, kBindingXEqJim), 1u);
    EXPECT_EQ(count_substr(out, kBindingXEqAnn), 1u);
    EXPECT_THAT(out, HasSubstr("REFUTED"));
}

TEST_P(CommandHandlerParamTest, ReachabilityGoalVarsAppearInBindings) {
    const std::string out = run_handler_capture(
        "cli/examples/reachability/db.chc", "reach(0, X), reach(X, Y)", kMaxResolutions,
        stdin_for_solutions(4), GetParam());
    EXPECT_GE(count_substr(out, "SOLVED"), 3u);
    EXPECT_GE(count_substr(out, kBindingXEqOne), 1u);
    EXPECT_GE(count_substr(out, kBindingXEqTwo), 1u);
    EXPECT_GE(count_substr(out, kBindingYEqTwo), 1u);
    EXPECT_GE(count_substr(out, kBindingYEqThree), 1u);
    EXPECT_THAT(out, HasSubstr("REFUTED"));
}

TEST_P(CommandHandlerParamTest, MemberListEnumeratesThreeDistinctBindings) {
    static constexpr size_t kExpectedSolutions = 3;
    const std::string out = run_handler_capture(
        "cli/examples/member/db.chc", "member(X, [a, b, c])", kMaxResolutions,
        stdin_for_solutions(kExpectedSolutions + 1), GetParam());
    EXPECT_EQ(count_substr(out, "SOLVED"), kExpectedSolutions);
    EXPECT_EQ(count_substr(out, kBindingXEqA), 1u);
    EXPECT_EQ(count_substr(out, kBindingXEqB), 1u);
    EXPECT_EQ(count_substr(out, kBindingXEqC), 1u);
    EXPECT_THAT(out, HasSubstr("REFUTED"));
}

TEST_P(CommandHandlerParamTest, SatOrTrueEnumeratesMultipleSolutions) {
    const std::string out = run_handler_capture(
        "cli/examples/sat/db.chc", "or(true, X, Y)", kMaxResolutions, stdin_for_solutions(8),
        GetParam());
    EXPECT_EQ(count_substr(out, "SOLVED"), 2u);
    EXPECT_EQ(count_substr(out, kBindingXEqTrue), 1u);
    EXPECT_EQ(count_substr(out, kBindingYEqTrue), 2u);
    EXPECT_EQ(count_substr(out, kBindingXEqFalse), 1u);
    EXPECT_THAT(out, HasSubstr("REFUTED"));
}

TEST_P(CommandHandlerParamTest, SolveLoopPromptsBetweenSolutions) {
    const std::string out = run_handler_capture(
        "cli/examples/member/db.chc", "member(X, [a, b, c])", kMaxResolutions,
        stdin_for_solutions(3), GetParam());
    EXPECT_GE(count_substr(out, "[press Enter for next solution]"), 2u);
}

TEST_P(CommandHandlerParamTest, ExhaustionEndsWithRefuted) {
    const std::string out = run_handler_capture(
        "cli/examples/member/db.chc", "member(X, [a, b, c])", kMaxResolutions,
        stdin_for_solutions(10), GetParam());
    EXPECT_EQ(count_substr(out, "SOLVED"), 3u);
    EXPECT_EQ(count_substr(out, kBindingXEqA), 1u);
    EXPECT_EQ(count_substr(out, kBindingXEqB), 1u);
    EXPECT_EQ(count_substr(out, kBindingXEqC), 1u);
    EXPECT_THAT(out, HasSubstr("REFUTED"));
    EXPECT_GT(out.rfind("REFUTED"), out.rfind("SOLVED"));
}
