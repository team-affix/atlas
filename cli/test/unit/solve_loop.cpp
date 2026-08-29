// solve_loop: interval orchestration for sim progress. Mocks IOnSim, IPrintStats,
// and IFinishPrintingLine separately; asserts they are called at interval
// boundaries and before SOLVED/REFUTED output, never when interval is zero.
// Pause/resume timer bracket the Enter wait after a solution.

#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/solve_loop.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/lemma.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;

struct MockRuntime {
    MOCK_METHOD(bool, next, ());
    MOCK_METHOD(bool, solved, (), (const));
    MOCK_METHOD(const expr*, normalize, (framed_expr));
    MOCK_METHOD(lemma, derive_decision_lemma, (), (const));
    MOCK_METHOD(lemma, derive_resolution_lemma, (), (const));
};

struct MockExprPrinter {
    MOCK_METHOD(void, print, (const expr*), (const));
};

struct MockPrintBindings {
    MOCK_METHOD(void, print, (MockRuntime&, MockExprPrinter&, expr_pool&,
        (const std::map<std::string, uint32_t>&)));
};

struct MockOnSim {
    MOCK_METHOD(void, on_sim, ());
};

struct MockPrintStats {
    MOCK_METHOD(void, print, ());
};

struct MockFinishPrintingLine {
    MOCK_METHOD(void, finish_line, ());
};

struct MockPauseTimer {
    MOCK_METHOD(void, pause, ());
};

struct MockResumeTimer {
    MOCK_METHOD(void, resume, ());
};

using TestSolveLoop = solve_loop<
    MockRuntime, MockExprPrinter, MockPrintBindings,
    MockOnSim, MockPrintStats, MockFinishPrintingLine,
    MockPauseTimer, MockResumeTimer>;

struct SolveLoopTest : public ::testing::Test {
    std::optional<expr_pool> pool;
    MockPrintBindings bindings;
    MockOnSim on_sim;
    MockPrintStats print_stats;
    MockFinishPrintingLine finish_printing_line;
    MockPauseTimer pause_timer;
    MockResumeTimer resume_timer;
    MockRuntime runtime;
    MockExprPrinter printer;
    std::map<std::string, uint32_t> var_name_to_idx;

    SolveLoopTest() { pool.emplace(); }

    void run_loop(size_t interval) {
        TestSolveLoop loop{bindings, on_sim, print_stats, finish_printing_line,
                           pause_timer, resume_timer, interval};
        loop.run(runtime, printer, *pool, var_name_to_idx);
    }

    void redirect_cin(const std::string& input) {
        fake_in_.str(input);
        fake_in_.clear();
        old_in_ = std::cin.rdbuf(fake_in_.rdbuf());
    }

    void restore_cin() {
        if (old_in_) { std::cin.rdbuf(old_in_); old_in_ = nullptr; }
    }

    ~SolveLoopTest() override { restore_cin(); }

private:
    std::istringstream fake_in_;
    std::streambuf* old_in_ = nullptr;
};

namespace {

constexpr size_t kInterval100  = 100;
constexpr size_t kTotalSims250 = 250;

}  // namespace

TEST_F(SolveLoopTest, DisabledWhenIntervalZero) {
    size_t next_calls = 0;
    EXPECT_CALL(resume_timer, resume()).Times(1);
    EXPECT_CALL(runtime, next()).WillRepeatedly([&] { return ++next_calls <= kTotalSims250; });
    EXPECT_CALL(runtime, solved()).WillRepeatedly(Return(false));
    EXPECT_CALL(on_sim, on_sim()).Times(0);
    EXPECT_CALL(print_stats, print()).Times(0);
    EXPECT_CALL(finish_printing_line, finish_line()).Times(0);
    EXPECT_CALL(pause_timer, pause()).Times(0);

    run_loop(0);
}

TEST_F(SolveLoopTest, PrintsOnIntervalBoundary) {
    size_t next_calls = 0;
    EXPECT_CALL(resume_timer, resume()).Times(1);
    EXPECT_CALL(runtime, next()).WillRepeatedly([&] { return ++next_calls <= kTotalSims250; });
    EXPECT_CALL(runtime, solved()).WillRepeatedly(Return(false));
    EXPECT_CALL(on_sim, on_sim()).Times(kTotalSims250);
    EXPECT_CALL(print_stats, print()).Times(3);
    EXPECT_CALL(finish_printing_line, finish_line()).Times(1);

    run_loop(kInterval100);
}

TEST_F(SolveLoopTest, DoesNotPrintWhenNextReturnsFalse) {
    EXPECT_CALL(resume_timer, resume()).Times(1);
    EXPECT_CALL(runtime, next()).WillOnce(Return(false));
    EXPECT_CALL(on_sim, on_sim()).Times(0);
    EXPECT_CALL(print_stats, print()).Times(0);
    EXPECT_CALL(finish_printing_line, finish_line()).Times(0);

    run_loop(kInterval100);
}

TEST_F(SolveLoopTest, FinishLineBeforeSolvedAndPauseAroundEnter) {
    redirect_cin("\n");

    size_t next_calls = 0;
    EXPECT_CALL(runtime, next()).WillRepeatedly([&] { return ++next_calls <= 3; });
    EXPECT_CALL(runtime, solved())
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    EXPECT_CALL(on_sim, on_sim()).Times(3);
    EXPECT_CALL(print_stats, print()).Times(4);
    {
        InSequence seq;
        EXPECT_CALL(resume_timer, resume());
        EXPECT_CALL(finish_printing_line, finish_line());
        EXPECT_CALL(bindings, print(_, _, _, _));
        EXPECT_CALL(pause_timer, pause());
        EXPECT_CALL(resume_timer, resume());
        EXPECT_CALL(finish_printing_line, finish_line());
    }

    run_loop(1);
}

TEST_F(SolveLoopTest, PauseStillBracketsEnterWhenIntervalZero) {
    redirect_cin("\n");

    size_t next_calls = 0;
    EXPECT_CALL(runtime, next()).WillRepeatedly([&] { return ++next_calls <= 1; });
    EXPECT_CALL(runtime, solved()).WillOnce(Return(true));
    EXPECT_CALL(on_sim, on_sim()).Times(0);
    {
        InSequence seq;
        EXPECT_CALL(resume_timer, resume());
        EXPECT_CALL(bindings, print(_, _, _, _));
        EXPECT_CALL(pause_timer, pause());
        EXPECT_CALL(resume_timer, resume());
    }

    run_loop(0);
}
