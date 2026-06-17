// solve_loop: interval orchestration for cumulative sim progress. Mocks i_print_progress
// and i_runtime; asserts print/finish_line are called at interval boundaries and before
// SOLVED/REFUTED output, never when interval is zero.

#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/locator.hpp"
#include "infrastructure/solve_loop.hpp"
#include "infrastructure/trail.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"
#include "interfaces/i_derive_decision_lemma.hpp"
#include "interfaces/i_derive_resolution_lemma.hpp"
#include "interfaces/i_expr_printer.hpp"
#include "interfaces/i_print_bindings.hpp"
#include "interfaces/i_print_progress.hpp"
#include "interfaces/i_runtime.hpp"
#include "value_objects/lemma.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;

struct MockPrintProgress : public i_print_progress {
    MOCK_METHOD(void, print, (size_t total_sims), (override));
    MOCK_METHOD(void, finish_line, (), (override));
};

struct MockPrintBindings : public i_print_bindings {
    MOCK_METHOD(
        void,
        print,
        (i_runtime&, i_expr_printer&, expr_pool&, (const std::map<std::string, uint32_t>&)),
        (override));
};

struct MockExprPrinter : public i_expr_printer {
    MOCK_METHOD(void, print, (const expr*), (const, override));
};

struct MockRuntime : public i_runtime {
    MOCK_METHOD(bool, next, (), (override));
    MOCK_METHOD(bool, solved, (), (const, override));
    MOCK_METHOD(const expr*, normalize, (framed_expr), (override));
    MOCK_METHOD(lemma, derive_decision_lemma, (), (const, override));
    MOCK_METHOD(lemma, derive_resolution_lemma, (), (const, override));
};

struct SolveLoopTest : public ::testing::Test {
    locator loc;
    trail trail_;
    std::optional<expr_pool> pool;
    MockPrintBindings bindings;
    MockPrintProgress progress;
    MockRuntime runtime;
    MockExprPrinter printer;
    std::map<std::string, uint32_t> var_name_to_idx;

    SolveLoopTest() {
        pool.emplace();
    }

    void run_loop(size_t interval) {
        solve_loop loop{bindings, progress, interval};
        loop.run(runtime, printer, *pool, var_name_to_idx);
    }

    void redirect_cin(const std::string& input) {
        fake_in_.str(input);
        fake_in_.clear();
        old_in_ = std::cin.rdbuf(fake_in_.rdbuf());
    }

    void restore_cin() {
        std::cin.rdbuf(old_in_);
    }

    ~SolveLoopTest() override { restore_cin(); }

private:
    std::istringstream fake_in_;
    std::streambuf* old_in_ = nullptr;
};

namespace {

constexpr size_t kInterval100 = 100;
constexpr size_t kTotalSims250 = 250;

}  // namespace

TEST_F(SolveLoopTest, DisabledWhenIntervalZero) {
    size_t next_calls = 0;
    EXPECT_CALL(runtime, next()).WillRepeatedly([&] { return ++next_calls <= kTotalSims250; });
    EXPECT_CALL(runtime, solved()).WillRepeatedly(Return(false));
    EXPECT_CALL(progress, print(_)).Times(0);
    EXPECT_CALL(progress, finish_line()).Times(0);

    run_loop(0);
}

TEST_F(SolveLoopTest, PrintsOnIntervalBoundary) {
    size_t next_calls = 0;
    EXPECT_CALL(runtime, next()).WillRepeatedly([&] { return ++next_calls <= kTotalSims250; });
    EXPECT_CALL(runtime, solved()).WillRepeatedly(Return(false));
    EXPECT_CALL(progress, print(kInterval100));
    EXPECT_CALL(progress, print(kInterval100 * 2));
    EXPECT_CALL(progress, print(kTotalSims250));
    EXPECT_CALL(progress, finish_line()).Times(1);

    run_loop(kInterval100);
}

TEST_F(SolveLoopTest, PrintsFinalCountWhenNotOnBoundary) {
    size_t next_calls = 0;
    EXPECT_CALL(runtime, next()).WillRepeatedly([&] { return ++next_calls <= kTotalSims250; });
    EXPECT_CALL(runtime, solved()).WillRepeatedly(Return(false));
    EXPECT_CALL(progress, print(kInterval100));
    EXPECT_CALL(progress, print(kInterval100 * 2));
    EXPECT_CALL(progress, print(kTotalSims250));
    EXPECT_CALL(progress, finish_line()).Times(1);

    run_loop(kInterval100);
}

TEST_F(SolveLoopTest, FinishLineBeforeSolved) {
    redirect_cin("\n");

    size_t next_calls = 0;
    EXPECT_CALL(runtime, next()).WillRepeatedly([&] { return ++next_calls <= 3; });
    EXPECT_CALL(runtime, solved())
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    {
        InSequence seq;
        EXPECT_CALL(progress, print(1));
        EXPECT_CALL(progress, print(2));
        EXPECT_CALL(progress, print(3));
        EXPECT_CALL(progress, finish_line());
        EXPECT_CALL(bindings, print(_, _, _, _));
        EXPECT_CALL(progress, finish_line());
    }

    run_loop(1);
}

TEST_F(SolveLoopTest, DoesNotPrintWhenNextReturnsFalse) {
    EXPECT_CALL(runtime, next()).WillOnce(Return(false));
    EXPECT_CALL(progress, print(_)).Times(0);
    EXPECT_CALL(progress, finish_line()).Times(0);

    run_loop(kInterval100);
}
