// basic_command_handler: parse CHC file + goal string, wire basic_solver_session.
// Construction tests across cli/examples; print_bindings for named goal vars.

#include <functional>
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "cli/hpp/basic_command_handler.hpp"
#include "interfaces/i_get_rule.hpp"
#include "value_objects/lineage.hpp"

using ::testing::Contains;
using ::testing::Ge;
using ::testing::IsEmpty;
using ::testing::Key;
using ::testing::SizeIs;

static const rule* rule_at(const db& database, rule_id id) {
    return static_cast<const i_get_rule&>(database).get(id);
}

static size_t rule_count(const db& database) {
    size_t n = 0;
    while (true) {
        try {
            rule_at(database, static_cast<rule_id>(n));
            ++n;
        } catch (const std::out_of_range&) {
            return n;
        }
    }
}

static std::string capture_cout(std::function<void()> fn) {
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    fn();
    std::cout.rdbuf(old);
    return oss.str();
}

struct BasicCommandHandlerTest : public ::testing::Test {
    static constexpr size_t kMaxResolutions = 1000;
    static constexpr uint32_t kSeed         = 0;
};

TEST_F(BasicCommandHandlerTest, ConstructsEqGround) {
    basic_command_handler h("cli/examples/eq/db.chc", "eq(a, a)", kMaxResolutions, kSeed);
    EXPECT_EQ(rule_count(h.test_database()), 1u);
    EXPECT_EQ(h.test_initial_goals().count(), 1u);
    EXPECT_EQ(h.test_var_name_to_idx().size(), 0u);
}

TEST_F(BasicCommandHandlerTest, ConstructsAncestor) {
    basic_command_handler h("cli/examples/ancestor/db.chc", "ancestor(tom, bob)", kMaxResolutions, kSeed);
    EXPECT_EQ(rule_count(h.test_database()), 7u);
    for (rule_id id = 0; id < 5; ++id)
        EXPECT_THAT(rule_at(h.test_database(), id)->body, IsEmpty());
    EXPECT_THAT(rule_at(h.test_database(), rule_id{5})->body, SizeIs(1));
    EXPECT_THAT(rule_at(h.test_database(), rule_id{6})->body, SizeIs(2));
}

TEST_F(BasicCommandHandlerTest, ConstructsReachability) {
    basic_command_handler h("cli/examples/reachability/db.chc", "reach(0, X)", kMaxResolutions, kSeed);
    EXPECT_THAT(h.test_var_name_to_idx(), Contains(Key("X")));
    EXPECT_GE(h.test_parse_var_peek(), 1u);
}

TEST_F(BasicCommandHandlerTest, ConstructsMemberList) {
    basic_command_handler h("cli/examples/member/db.chc", "member(X, [a, b, c])", kMaxResolutions, kSeed);
    EXPECT_THAT(h.test_var_name_to_idx(), Contains(Key("X")));
    EXPECT_EQ(h.test_initial_goals().count(), 1u);
}

TEST_F(BasicCommandHandlerTest, ConstructsSat) {
    basic_command_handler h("cli/examples/sat/db.chc", "or(true, X, Y)", kMaxResolutions, kSeed);
    EXPECT_THAT(h.test_var_name_to_idx(), Contains(Key("X")));
    EXPECT_THAT(h.test_var_name_to_idx(), Contains(Key("Y")));
}

TEST_F(BasicCommandHandlerTest, ConstructsTrees) {
    basic_command_handler h("cli/examples/trees/db.chc", "in(X, bin(nil, nil))", kMaxResolutions, kSeed);
    EXPECT_THAT(h.test_var_name_to_idx(), Contains(Key("X")));
    EXPECT_EQ(h.test_initial_goals().count(), 1u);
}

TEST_F(BasicCommandHandlerTest, ConstructsArithmetic) {
    basic_command_handler h("cli/examples/arithmetic/db.chc", "even(zero)", kMaxResolutions, kSeed);
    EXPECT_GT(rule_count(h.test_database()), 0u);
    EXPECT_EQ(h.test_initial_goals().count(), 1u);
}

TEST_F(BasicCommandHandlerTest, ConstructsExpr) {
    basic_command_handler h("cli/examples/expr/db.chc", "eval(E, zero)", kMaxResolutions, kSeed);
    EXPECT_THAT(h.test_var_name_to_idx(), Contains(Key("E")));
}

TEST_F(BasicCommandHandlerTest, BadFileThrows) {
    EXPECT_THROW(
        basic_command_handler("cli/examples/nonexistent/db.chc", "p(X)", kMaxResolutions, kSeed),
        std::runtime_error);
}

TEST_F(BasicCommandHandlerTest, BadGoalThrows) {
    EXPECT_THROW(
        basic_command_handler("cli/examples/eq/db.chc", ":-", kMaxResolutions, kSeed),
        std::runtime_error);
}

TEST_F(BasicCommandHandlerTest, PrintBindingsMentionsVars) {
    basic_command_handler h("cli/examples/reachability/db.chc", "reach(0, X), reach(X, Y)", kMaxResolutions, kSeed);
    const std::string out = capture_cout([&]() { h.test_print_bindings(); });
    EXPECT_THAT(out, testing::HasSubstr("X"));
    EXPECT_THAT(out, testing::HasSubstr("Y"));
}
