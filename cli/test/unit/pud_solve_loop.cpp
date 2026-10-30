#include <map>
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_solve_loop.hpp"
#include "value_objects/expr.hpp"

using ::testing::Return;

struct MockPudRuntime {
    MOCK_METHOD(bool, next, ());
    MOCK_METHOD(const expr*, solution, (), (const));
    MOCK_METHOD(size_t, solution_depth, (), (const));
};

struct MockPudExprPrinter {
    MOCK_METHOD(void, print, (const expr*), (const));
};

struct MockPudOnTick {
    MOCK_METHOD(void, on_tick, ());
};

struct MockPudPrintStats {
    MOCK_METHOD(void, print, ());
};

struct MockPudFinishPrintingLine {
    MOCK_METHOD(void, finish_line, ());
};

struct MockPudPauseTimer {
    MOCK_METHOD(void, pause, ());
};

struct MockPudResumeTimer {
    MOCK_METHOD(void, resume, ());
};

using TestPudSolveLoop = pud_solve_loop<
    MockPudRuntime, MockPudExprPrinter, MockPudOnTick, MockPudPrintStats,
    MockPudFinishPrintingLine, MockPudPauseTimer, MockPudResumeTimer>;

struct PudSolveLoopTest : public ::testing::Test {
    PudSolveLoopTest();
    MockPudRuntime runtime;
    MockPudExprPrinter printer;
    MockPudOnTick on_tick;
    MockPudPrintStats print_stats;
    MockPudFinishPrintingLine finish_printing_line;
    MockPudPauseTimer pause_timer;
    MockPudResumeTimer resume_timer;

    void run_loop(size_t interval) {
        TestPudSolveLoop loop{
            on_tick, print_stats, finish_printing_line, pause_timer, resume_timer, interval};
        loop.run(runtime, printer);
    }

    void redirect_cin(const std::string& input) {
        fake_in_.str(input);
        fake_in_.clear();
        old_in_ = std::cin.rdbuf(fake_in_.rdbuf());
    }

    ~PudSolveLoopTest() override {
        if (old_in_ != nullptr)
            std::cin.rdbuf(old_in_);
    }

private:
    std::istringstream fake_in_;
    std::streambuf* old_in_;
};

PudSolveLoopTest::PudSolveLoopTest()
    : old_in_(nullptr) {}

TEST_F(PudSolveLoopTest, RefutesWhenTheFirstTickStops) {
    EXPECT_CALL(resume_timer, resume()).Times(1);
    EXPECT_CALL(runtime, next()).WillOnce(Return(false));
    EXPECT_CALL(on_tick, on_tick()).Times(0);
    EXPECT_CALL(print_stats, print()).Times(0);

    run_loop(100);
}

TEST_F(PudSolveLoopTest, PrintsTheSolutionHeadAndDepth) {
    redirect_cin("\n");
    expr fact{expr::functor{2, {}}};
    EXPECT_CALL(resume_timer, resume()).Times(2);
    EXPECT_CALL(runtime, next()).WillOnce(Return(true)).WillOnce(Return(false));
    EXPECT_CALL(runtime, solution()).WillOnce(Return(&fact));
    EXPECT_CALL(runtime, solution_depth()).WillOnce(Return(3));
    EXPECT_CALL(printer, print(&fact)).Times(1);
    EXPECT_CALL(pause_timer, pause()).Times(1);
    EXPECT_CALL(on_tick, on_tick()).Times(0);

    run_loop(0);
}
