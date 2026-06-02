// basic_command_handler: public API only — construction (parse + session wiring) and
// operator() solve loop output. Example DBs under cli/examples/.

#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "cli/hpp/basic_command_handler.hpp"

using ::testing::HasSubstr;

struct BasicCommandHandlerTest : public ::testing::Test {
    static constexpr size_t kMaxResolutions = 1000;
    static constexpr uint32_t kSeed         = 0;
};

static std::string run_handler(
    const std::string& file,
    const std::string& goal,
    size_t max_resolutions,
    const std::string& stdin_lines) {
    std::ostringstream captured;
    std::istringstream fake_in(stdin_lines);
    std::streambuf* old_out = std::cout.rdbuf(captured.rdbuf());
    std::streambuf* old_in  = std::cin.rdbuf(fake_in.rdbuf());
    basic_command_handler{file, goal, max_resolutions, BasicCommandHandlerTest::kSeed}();
    std::cout.rdbuf(old_out);
    std::cin.rdbuf(old_in);
    return captured.str();
}

// ---------------------------------------------------------------------------
// Construction (parse + session emplace must not throw)
// ---------------------------------------------------------------------------

TEST_F(BasicCommandHandlerTest, ConstructsFromEqExample) {
    EXPECT_NO_THROW(basic_command_handler("cli/examples/eq/db.chc", "eq(a, a)", kMaxResolutions, kSeed));
}

TEST_F(BasicCommandHandlerTest, ConstructsFromAncestorExample) {
    EXPECT_NO_THROW(basic_command_handler("cli/examples/ancestor/db.chc", "ancestor(tom, bob)", kMaxResolutions, kSeed));
}

TEST_F(BasicCommandHandlerTest, ConstructsFromReachabilityExample) {
    EXPECT_NO_THROW(basic_command_handler("cli/examples/reachability/db.chc", "reach(0, X)", kMaxResolutions, kSeed));
}

TEST_F(BasicCommandHandlerTest, ConstructsFromMemberExample) {
    EXPECT_NO_THROW(basic_command_handler("cli/examples/member/db.chc", "member(X, [a, b, c])", kMaxResolutions, kSeed));
}

TEST_F(BasicCommandHandlerTest, ConstructsFromSatExample) {
    EXPECT_NO_THROW(basic_command_handler("cli/examples/sat/db.chc", "or(true, X, Y)", kMaxResolutions, kSeed));
}

TEST_F(BasicCommandHandlerTest, ConstructsFromTreesExample) {
    EXPECT_NO_THROW(basic_command_handler("cli/examples/trees/db.chc", "in(X, bin(nil, nil))", kMaxResolutions, kSeed));
}

TEST_F(BasicCommandHandlerTest, ConstructsFromArithmeticExample) {
    EXPECT_NO_THROW(basic_command_handler("cli/examples/arithmetic/db.chc", "even(zero)", kMaxResolutions, kSeed));
}

TEST_F(BasicCommandHandlerTest, ConstructsFromExprExample) {
    EXPECT_NO_THROW(basic_command_handler("cli/examples/expr/db.chc", "eval(E, zero)", kMaxResolutions, kSeed));
}

TEST_F(BasicCommandHandlerTest, BadFileThrowsOnConstruction) {
    EXPECT_THROW(
        basic_command_handler("cli/examples/nonexistent/db.chc", "p(X)", kMaxResolutions, kSeed),
        std::runtime_error);
}

TEST_F(BasicCommandHandlerTest, BadGoalThrowsOnConstruction) {
    EXPECT_THROW(
        basic_command_handler("cli/examples/eq/db.chc", ":-", kMaxResolutions, kSeed),
        std::runtime_error);
}

// ---------------------------------------------------------------------------
// operator() — solve loop via stdout
// ---------------------------------------------------------------------------

TEST_F(BasicCommandHandlerTest, EqGroundSolveThenRefute) {
    const std::string out = run_handler("cli/examples/eq/db.chc", "eq(a, a)", kMaxResolutions, "\n");
    EXPECT_THAT(out, HasSubstr("SOLVED"));
    EXPECT_THAT(out, HasSubstr("REFUTED"));
}

TEST_F(BasicCommandHandlerTest, AncestorGoalPrintsVarBinding) {
    const std::string out = run_handler(
        "cli/examples/ancestor/db.chc", "ancestor(tom, X)", kMaxResolutions, "\n\n\n\n\n\n");
    EXPECT_THAT(out, HasSubstr("SOLVED"));
    EXPECT_THAT(out, HasSubstr("X"));
    EXPECT_THAT(out, HasSubstr("bob"));
}

TEST_F(BasicCommandHandlerTest, ReachabilityGoalVarsAppearInBindings) {
    const std::string out = run_handler(
        "cli/examples/reachability/db.chc", "reach(0, X), reach(X, Y)", kMaxResolutions, "\n\n\n\n");
    EXPECT_THAT(out, HasSubstr("SOLVED"));
    EXPECT_THAT(out, HasSubstr("X"));
    EXPECT_THAT(out, HasSubstr("Y"));
}
