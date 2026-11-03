#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_solve_loop.hpp"
#include "value_objects/expr.hpp"

using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockPudRuntime {
    MOCK_METHOD(bool, next, ());
    MOCK_METHOD(const expr*, solution, (), (const));
    MOCK_METHOD(size_t, solution_depth, (), (const));
    MOCK_METHOD(pud_search_stop_reason, stop_reason, (), (const));
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
struct MockPudPauseTimer {
    MOCK_METHOD(void, pause, ());
};
struct MockPudResumeTimer {
    MOCK_METHOD(void, resume, ());
};
struct MockPudTrySaveModules {
    MOCK_METHOD(void, try_save, ());
};
struct MockPudTrySaveModulesFinal {
    MOCK_METHOD(void, try_save_final, ());
};

using test_loop_t = pud_solve_loop<
    MockPudRuntime, MockPudRuntime, MockPudRuntime, MockPudRuntime, NiceMock<MockPudExprPrinter>,
    NiceMock<MockPudOnTick>, NiceMock<MockPudPrintStats>, NiceMock<MockPudPauseTimer>, NiceMock<MockPudResumeTimer>,
    NiceMock<MockPudTrySaveModules>, NiceMock<MockPudTrySaveModulesFinal>>;

// Captures std::cout and feeds std::cin for the duration of each test.
struct PudSolveLoopTest : public ::testing::Test {
    PudSolveLoopTest();
    ~PudSolveLoopTest() override;
    NiceMock<MockPudRuntime> runtime;
    NiceMock<MockPudExprPrinter> printer;
    NiceMock<MockPudOnTick> on_tick;
    NiceMock<MockPudPrintStats> print_stats;
    NiceMock<MockPudPauseTimer> pause_timer;
    NiceMock<MockPudResumeTimer> resume_timer;
    NiceMock<MockPudTrySaveModules> saver;
    NiceMock<MockPudTrySaveModulesFinal> final_saver;
    expr fact{expr::functor{2, {}}};

    std::string run_loop(size_t interval) {
        test_loop_t loop{
            runtime, runtime, runtime, runtime, printer, on_tick, print_stats, pause_timer, resume_timer, saver,
            final_saver, interval};
        loop.run();
        return captured_out_.str();
    }

private:
    std::istringstream fake_in_;
    std::ostringstream captured_out_;
    std::streambuf* old_in_;
    std::streambuf* old_out_;
};

PudSolveLoopTest::PudSolveLoopTest()
    : fake_in_("\n\n\n")
    , captured_out_()
    , old_in_(std::cin.rdbuf(fake_in_.rdbuf()))
    , old_out_(std::cout.rdbuf(captured_out_.rdbuf())) {
    ON_CALL(runtime, next()).WillByDefault(Return(false));
    ON_CALL(runtime, solution()).WillByDefault(Return(nullptr));
    ON_CALL(runtime, stop_reason()).WillByDefault(Return(pud_search_stop_reason::refuted));
}

PudSolveLoopTest::~PudSolveLoopTest() {
    std::cout.rdbuf(old_out_);
    std::cin.rdbuf(old_in_);
}

}

TEST_F(PudSolveLoopTest, ARefutedSessionEndsWithREFUTED) {
    EXPECT_EQ(run_loop(0), "REFUTED\n");
}

TEST_F(PudSolveLoopTest, AnExhaustedSessionEndsWithEXHAUSTED) {
    ON_CALL(runtime, stop_reason()).WillByDefault(Return(pud_search_stop_reason::exhausted));
    EXPECT_EQ(run_loop(0), "EXHAUSTED\n");
}

TEST_F(PudSolveLoopTest, ASpentBudgetEndsWithoutAFinalLine) {
    ON_CALL(runtime, stop_reason()).WillByDefault(Return(pud_search_stop_reason::budget_spent));
    EXPECT_EQ(run_loop(0), "");
}

TEST_F(PudSolveLoopTest, EachSolutionIsPrintedWithItsDepthAndWaitsForEnter) {
    EXPECT_CALL(runtime, next()).WillOnce(Return(true)).WillOnce(Return(false));
    EXPECT_CALL(runtime, solution()).WillOnce(Return(&fact));
    EXPECT_CALL(runtime, solution_depth()).WillOnce(Return(3));
    EXPECT_CALL(printer, print(&fact));
    EXPECT_CALL(pause_timer, pause());
    EXPECT_CALL(resume_timer, resume()).Times(2);
    EXPECT_EQ(run_loop(0), "SOLVED\n\ndepth 3\n[press Enter for next solution]REFUTED\n");
}

TEST_F(PudSolveLoopTest, ATickWithoutASolutionPrintsNothing) {
    EXPECT_CALL(runtime, next()).WillOnce(Return(true)).WillOnce(Return(false));
    EXPECT_CALL(printer, print(::testing::_)).Times(0);
    EXPECT_EQ(run_loop(0), "REFUTED\n");
}

TEST_F(PudSolveLoopTest, ProgressIsPrintedAtEachIntervalAndOnceAtTheEnd) {
    EXPECT_CALL(runtime, next())
        .WillOnce(Return(true)).WillOnce(Return(true)).WillOnce(Return(true)).WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_CALL(on_tick, on_tick()).Times(4);
    EXPECT_CALL(print_stats, print()).Times(3);
    run_loop(2);
}

TEST_F(PudSolveLoopTest, NoProgressIsPrintedWhenDisabledOrWithoutTicks) {
    EXPECT_CALL(on_tick, on_tick()).Times(0);
    EXPECT_CALL(print_stats, print()).Times(0);
    run_loop(100);
}

TEST_F(PudSolveLoopTest, ModulesGetASaveChanceAfterEveryTickBeforeWaitingForEnter) {
    EXPECT_CALL(runtime, next()).WillOnce(Return(true)).WillOnce(Return(true)).WillOnce(Return(false));
    EXPECT_CALL(runtime, solution()).WillOnce(Return(&fact)).WillOnce(Return(nullptr));
    InSequence order;
    EXPECT_CALL(saver, try_save());
    EXPECT_CALL(pause_timer, pause());
    EXPECT_CALL(saver, try_save());
    EXPECT_CALL(final_saver, try_save_final());
    run_loop(0);
}

TEST_F(PudSolveLoopTest, TheFinalSaveChanceComesOnceBeforeTheLastLine) {
    EXPECT_CALL(saver, try_save()).Times(0);
    EXPECT_CALL(final_saver, try_save_final()).Times(1);
    EXPECT_EQ(run_loop(0), "REFUTED\n");
}
