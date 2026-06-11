// command_handler: parameterized tests for basic_command_handler and ridge_command_handler.
// Example DBs under cli/examples/.

#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/basic_command_handler.hpp"
#include "infrastructure/ridge_command_handler.hpp"

using ::testing::HasSubstr;

enum class cli_solver_kind { basic, ridge };

struct CommandHandlerParamTest : public ::testing::TestWithParam<cli_solver_kind> {
    static constexpr size_t kMaxResolutions       = 1000;
    static constexpr uint32_t kSeed               = 0;
    static constexpr double kExplorationConstant  = 1.414;
};

INSTANTIATE_TEST_SUITE_P(
    AllSolvers,
    CommandHandlerParamTest,
    ::testing::Values(cli_solver_kind::basic, cli_solver_kind::ridge),
    [](const auto& info) {
        return info.param == cli_solver_kind::basic ? "basic" : "ridge";
    });

namespace {

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
        "cli/examples/trees/db.chc", "in(X, bin(nil, nil))", kMaxResolutions, GetParam()));
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

TEST_P(CommandHandlerParamTest, EqGroundSolveThenRefute) {
    const std::string out = run_handler_capture(
        "cli/examples/eq/db.chc", "eq(a, a)", kMaxResolutions, "\n", GetParam());
    EXPECT_THAT(out, HasSubstr("SOLVED"));
    EXPECT_THAT(out, HasSubstr("REFUTED"));
}

TEST_P(CommandHandlerParamTest, AncestorGoalPrintsVarBinding) {
    const std::string out = run_handler_capture(
        "cli/examples/ancestor/db.chc", "ancestor(tom, X)", kMaxResolutions, "\n\n\n\n\n\n", GetParam());
    EXPECT_THAT(out, HasSubstr("SOLVED"));
    EXPECT_THAT(out, HasSubstr("X"));
    EXPECT_THAT(out, HasSubstr("bob"));
}

TEST_P(CommandHandlerParamTest, ReachabilityGoalVarsAppearInBindings) {
    const std::string out = run_handler_capture(
        "cli/examples/reachability/db.chc", "reach(0, X), reach(X, Y)", kMaxResolutions, "\n\n\n\n", GetParam());
    EXPECT_THAT(out, HasSubstr("SOLVED"));
    EXPECT_THAT(out, HasSubstr("X"));
    EXPECT_THAT(out, HasSubstr("Y"));
}
