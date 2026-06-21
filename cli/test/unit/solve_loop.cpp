// solve_loop: interval orchestration for sim progress. Mocks IPrintProgress and
// IRuntime; asserts on_sim/print/finish_line are called at interval boundaries and
// before SOLVED/REFUTED output, never when interval is zero.

#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/solve_loop.hpp"
#include "infrastructure/trail.hpp"
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

struct MockPrintProgress {
    MOCK_METHOD(void, on_sim, ());
    MOCK_METHOD(void, print, ());
    MOCK_METHOD(void, finish_line, ());
};

using TestSolveLoop = solve_loop<MockRuntime, MockExprPrinter, MockPrintBindings, MockPrintProgress>;

struct SolveLoopTest : public ::testing::Test {
    std::optional<expr_pool> pool;
    MockPrintBindings bindings;
    MockPrintProgress progress;
    MockRuntime runtime;
    MockExprPrinter printer;
    std::map<std::string, uint32_t> var_name_to_idx;

    SolveLoopTest() { pool.emplace(); }

    void run_loop(size_t interval) {
        TestSolveLoop loop{bindings, progress, interval};
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
    EXPECT_CALL(runtime, next()).WillRepeatedly([&] { return ++next_calls <= kTotalSims250; });
    EXPECT_CALL(runtime, solved()).WillRepeatedly(Return(false));
    EXPECT_CALL(progress, on_sim()).Times(0);
    EXPECT_CALL(progress, print()).Times(0);
    EXPECT_CALL(progress, finish_line()).Times(0);

    run_loop(0);
}

TEST_F(SolveLoopTest, PrintsOnIntervalBoundary) {
    // 250 sims, interval 100: print() at sims 100, 200, and once more at end
    // (the end print is a no-op inside print_progress if sims_since_last_==0,
    // but MockPrintProgress just records the call regardless).
    size_t next_calls = 0;
    EXPECT_CALL(runtime, next()).WillRepeatedly([&] { return ++next_calls <= kTotalSims250; });
    EXPECT_CALL(runtime, solved()).WillRepeatedly(Return(false));
    EXPECT_CALL(progress, on_sim()).Times(kTotalSims250);
    EXPECT_CALL(progress, print()).Times(3);  // sim 100, sim 200, end-of-loop
    EXPECT_CALL(progress, finish_line()).Times(1);

    run_loop(kInterval100);
}

TEST_F(SolveLoopTest, DoesNotPrintWhenNextReturnsFalse) {
    EXPECT_CALL(runtime, next()).WillOnce(Return(false));
    EXPECT_CALL(progress, on_sim()).Times(0);
    EXPECT_CALL(progress, print()).Times(0);
    EXPECT_CALL(progress, finish_line()).Times(0);

    run_loop(kInterval100);
}

TEST_F(SolveLoopTest, FinishLineBeforeSolved) {
    // interval=1; 3 sims; solved on the 3rd.
    // Key contract: finish_line precedes bindings.print (SOLVED), followed by
    // a second finish_line at the end of the loop.
    redirect_cin("\n");

    size_t next_calls = 0;
    EXPECT_CALL(runtime, next()).WillRepeatedly([&] { return ++next_calls <= 3; });
    EXPECT_CALL(runtime, solved())
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    EXPECT_CALL(progress, on_sim()).Times(3);
    EXPECT_CALL(progress, print()).Times(4);  // sims 1, 2, 3, plus end-of-loop
    {
        InSequence seq;
        EXPECT_CALL(progress, finish_line());
        EXPECT_CALL(bindings, print(_, _, _, _));
        EXPECT_CALL(progress, finish_line());
    }

    run_loop(1);
}
