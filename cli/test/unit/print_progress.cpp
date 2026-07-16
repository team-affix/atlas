// print_progress<IRuntime>: stat-accumulating, TTY-aware progress reporter.
// Non-TTY path (captured stdout) wraps each line in newlines. Tests verify the
// output string contains the expected labelled fields; rates are instantaneous
// over the print window (idle time from note_idle_* is excluded).

#include <sstream>
#include <string>
#include <unistd.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/print_progress.hpp"

using ::testing::AtLeast;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Return;

struct MockRuntime {
    MOCK_METHOD(size_t, resolution_depth, (), (const));
    MOCK_METHOD(size_t, decision_depth,   (), (const));
};

struct PrintProgressTest : public ::testing::Test {
    std::ostringstream captured;
    std::streambuf* old_out      = nullptr;
    int saved_stdout_fd          = -1;
    int pipe_fds[2]              = {-1, -1};
    MockRuntime mock_rt;

    void SetUp() override {
        // Redirect fd 1 to a pipe so isatty(STDOUT_FILENO) returns false.
        saved_stdout_fd = dup(STDOUT_FILENO);
        ASSERT_NE(saved_stdout_fd, -1);
        ASSERT_EQ(pipe(pipe_fds), 0);
        ASSERT_NE(dup2(pipe_fds[1], STDOUT_FILENO), -1);
        old_out = std::cout.rdbuf(captured.rdbuf());

        ON_CALL(mock_rt, resolution_depth()).WillByDefault(Return(0));
        ON_CALL(mock_rt, decision_depth()).WillByDefault(Return(0));
    }

    void TearDown() override {
        std::cout.rdbuf(old_out);
        if (saved_stdout_fd != -1) {
            dup2(saved_stdout_fd, STDOUT_FILENO);
            close(saved_stdout_fd);
            saved_stdout_fd = -1;
        }
        if (pipe_fds[0] != -1) { close(pipe_fds[0]); pipe_fds[0] = -1; }
        if (pipe_fds[1] != -1) { close(pipe_fds[1]); pipe_fds[1] = -1; }
    }

    print_progress<MockRuntime> make_progress() {
        print_progress<MockRuntime> pp;
        pp.set_runtime(mock_rt);
        return pp;
    }
};

TEST_F(PrintProgressTest, NonTtyOutputContainsSimCount) {
    auto pp = make_progress();
    pp.on_sim();
    pp.print();

    EXPECT_THAT(captured.str(), HasSubstr("1 sims"));
}

TEST_F(PrintProgressTest, NonTtyOutputContainsAllFields) {
    auto pp = make_progress();
    pp.on_sim();
    pp.print();

    const std::string out = captured.str();
    EXPECT_THAT(out, HasSubstr("res "));
    EXPECT_THAT(out, HasSubstr("dec "));
    EXPECT_THAT(out, HasSubstr("sims/s"));
    EXPECT_THAT(out, HasSubstr("res/s"));
}

TEST_F(PrintProgressTest, PrintIsNoOpWhenNoSimsSinceLastPrint) {
    auto pp = make_progress();
    pp.on_sim();
    pp.print();
    const std::string after_first = captured.str();
    pp.print();  // no on_sim() since last print — should be a no-op

    EXPECT_EQ(captured.str(), after_first);
}

TEST_F(PrintProgressTest, FinishLineIsNoOpWhenNothingActive) {
    auto pp = make_progress();
    pp.finish_line();

    EXPECT_THAT(captured.str(), IsEmpty());
}

TEST_F(PrintProgressTest, FinishLineAfterNonTtyPrintClosesLine) {
    // print() leaves the cursor at end-of-content (no trailing \n) so that
    // decorators can append to the same line. finish_line() closes it.
    auto pp = make_progress();
    pp.on_sim();
    pp.print();
    const std::string before = captured.str();
    pp.finish_line();

    EXPECT_GT(captured.str().size(), before.size());
    EXPECT_EQ(captured.str().back(), '\n');
}

TEST_F(PrintProgressTest, SequentialPrintsAccumulateTotalSims) {
    auto pp = make_progress();
    for (size_t i = 0; i < 500; ++i) pp.on_sim();
    pp.print();
    for (size_t i = 0; i < 500; ++i) pp.on_sim();
    pp.print();

    const std::string out = captured.str();
    EXPECT_THAT(out, HasSubstr("500 sims"));
    EXPECT_THAT(out, HasSubstr("1000 sims"));
}

TEST_F(PrintProgressTest, PrintOutputContainsResolutionDepthField) {
    ON_CALL(mock_rt, resolution_depth()).WillByDefault(Return(20));
    ON_CALL(mock_rt, decision_depth()).WillByDefault(Return(5));

    auto pp = make_progress();
    pp.on_sim();
    pp.on_sim();
    pp.print();

    const std::string out = captured.str();
    EXPECT_THAT(out, HasSubstr("res "));
    EXPECT_THAT(out, HasSubstr("dec "));
}

TEST_F(PrintProgressTest, SimsSinceLastResetAfterPrint) {
    auto pp = make_progress();
    pp.on_sim();
    pp.on_sim();
    EXPECT_EQ(pp.sims_since_last(), 2u);
    pp.print();
    EXPECT_EQ(pp.sims_since_last(), 0u);
}

TEST_F(PrintProgressTest, IdleTimeIsExcludedFromRates) {
    ON_CALL(mock_rt, resolution_depth()).WillByDefault(Return(100));
    auto pp = make_progress();

    pp.on_sim();
    pp.note_idle_begin();
    usleep(200000);  // 200ms idle must not count as sim time
    pp.note_idle_end();
    pp.on_sim();
    pp.print();

    const std::string out = captured.str();
    EXPECT_THAT(out, HasSubstr("2 sims"));
    // With idle excluded, 2 sims in <<200ms of work should not collapse to 0.
    EXPECT_THAT(out, Not(HasSubstr("0 sims/s")));
    EXPECT_THAT(out, Not(HasSubstr("0 res/s")));
}
