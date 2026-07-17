// print_progress<IRuntime, IGetTimeSinceStart>: stat-accumulating, TTY-aware
// progress reporter. Rates use active-solving elapsed from time_since_start();
// each line includes TSS: X.Xs.

#include <chrono>
#include <sstream>
#include <string>
#include <unistd.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/print_progress.hpp"

using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

using dur = std::chrono::steady_clock::duration;

struct MockRuntime {
    MOCK_METHOD(size_t, resolution_depth, (), (const));
    MOCK_METHOD(size_t, decision_depth,   (), (const));
};

struct MockGetTimeSinceStart {
    using duration = dur;

    MOCK_METHOD(duration, time_since_start, (), (const));
};

}  // namespace

using test_print_progress_t = print_progress<MockRuntime, MockGetTimeSinceStart>;

struct PrintProgressTest : public ::testing::Test {
    std::ostringstream captured;
    std::streambuf* old_out      = nullptr;
    int saved_stdout_fd          = -1;
    int pipe_fds[2]              = {-1, -1};
    NiceMock<MockRuntime> mock_rt;
    NiceMock<MockGetTimeSinceStart> mock_tss;

    void SetUp() override {
        saved_stdout_fd = dup(STDOUT_FILENO);
        ASSERT_NE(saved_stdout_fd, -1);
        ASSERT_EQ(pipe(pipe_fds), 0);
        ASSERT_NE(dup2(pipe_fds[1], STDOUT_FILENO), -1);
        old_out = std::cout.rdbuf(captured.rdbuf());

        ON_CALL(mock_rt, resolution_depth()).WillByDefault(Return(0));
        ON_CALL(mock_rt, decision_depth()).WillByDefault(Return(0));
        ON_CALL(mock_tss, time_since_start()).WillByDefault(Return(dur::zero()));
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

    test_print_progress_t make_progress() {
        test_print_progress_t pp{mock_tss};
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
    EXPECT_CALL(mock_tss, time_since_start())
        .WillOnce(Return(dur::zero()))
        .WillOnce(Return(std::chrono::milliseconds{1234}));

    auto pp = make_progress();
    pp.on_sim();
    pp.print();

    const std::string out = captured.str();
    EXPECT_THAT(out, HasSubstr("res "));
    EXPECT_THAT(out, HasSubstr("dec "));
    EXPECT_THAT(out, HasSubstr("sims/s"));
    EXPECT_THAT(out, HasSubstr("res/s"));
    EXPECT_THAT(out, HasSubstr("TSS: 1.2s"));
}

TEST_F(PrintProgressTest, PrintIsNoOpWhenNoSimsSinceLastPrint) {
    auto pp = make_progress();
    pp.on_sim();
    pp.print();
    const std::string after_first = captured.str();
    pp.print();

    EXPECT_EQ(captured.str(), after_first);
}

TEST_F(PrintProgressTest, FinishLineIsNoOpWhenNothingActive) {
    auto pp = make_progress();
    pp.finish_line();

    EXPECT_THAT(captured.str(), IsEmpty());
}

TEST_F(PrintProgressTest, FinishLineAfterNonTtyPrintClosesLine) {
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

namespace {

size_t rate_before(const std::string& out, const std::string& suffix) {
    const auto end = out.rfind(suffix);
    if (end == std::string::npos || end == 0)
        return 0;
    size_t start = end;
    while (start > 0 && out[start - 1] >= '0' && out[start - 1] <= '9')
        --start;
    if (start == end)
        return 0;
    return static_cast<size_t>(std::stoul(out.substr(start, end - start)));
}

}  // namespace

TEST_F(PrintProgressTest, RatesUseActiveElapsedFromTimeSinceStart) {
    // Active elapsed for the print window is 0.1s even if wall time advanced more
    // while paused (TSS does not advance during pause).
    ON_CALL(mock_rt, resolution_depth()).WillByDefault(Return(100));
    EXPECT_CALL(mock_tss, time_since_start())
        .WillOnce(Return(dur::zero()))
        .WillOnce(Return(std::chrono::milliseconds{100}));

    auto pp = make_progress();
    pp.on_sim();
    pp.on_sim();
    pp.print();

    const std::string out = captured.str();
    EXPECT_THAT(out, HasSubstr("2 sims"));
    EXPECT_GE(rate_before(out, " sims/s"), 10u);
    EXPECT_GE(rate_before(out, " res/s"), 1000u);
}
