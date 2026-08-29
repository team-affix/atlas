// pause_poller: forwards inner print(), then maybe waits for a space toggle.
// Mocks IPrintStats, IIsTty, ITryReadByte, and IIdle so tests never touch the
// real terminal.

#include <iostream>
#include <optional>
#include <sstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pause_poller.hpp"

using ::testing::HasSubstr;
using ::testing::Return;

struct MockPrintStats {
    MOCK_METHOD(void, print, ());
};

struct MockIsTty {
    MOCK_METHOD(bool, is_tty, (), (const));
};

struct MockTryReadByte {
    MOCK_METHOD(std::optional<char>, try_read_byte, ());
};

struct MockIdle {
    MOCK_METHOD(void, idle, ());
};

using test_pause_poller_t =
    pause_poller<MockPrintStats, MockIsTty, MockTryReadByte, MockIdle>;

struct PausePollerTest : public ::testing::Test {
    MockPrintStats print_stats;
    MockIsTty is_tty;
    MockTryReadByte try_read_byte;
    MockIdle idle;
    test_pause_poller_t poller;

    PausePollerTest()
        : poller(print_stats, is_tty, try_read_byte, idle)
    {}
};

TEST_F(PausePollerTest, ForwardsOnlyWhenNotATty) {
    EXPECT_CALL(print_stats, print());
    EXPECT_CALL(is_tty, is_tty()).WillOnce(Return(false));
    EXPECT_CALL(try_read_byte, try_read_byte()).Times(0);
    EXPECT_CALL(idle, idle()).Times(0);

    poller.print();
}

TEST_F(PausePollerTest, IgnoresNonSpace) {
    EXPECT_CALL(print_stats, print());
    EXPECT_CALL(is_tty, is_tty()).WillOnce(Return(true));
    EXPECT_CALL(try_read_byte, try_read_byte())
        .WillOnce(Return(std::optional<char>{'x'}));
    EXPECT_CALL(idle, idle()).Times(0);

    poller.print();
}

TEST_F(PausePollerTest, IgnoresEmptyRead) {
    EXPECT_CALL(print_stats, print());
    EXPECT_CALL(is_tty, is_tty()).WillOnce(Return(true));
    EXPECT_CALL(try_read_byte, try_read_byte())
        .WillOnce(Return(std::optional<char>{}));
    EXPECT_CALL(idle, idle()).Times(0);

    poller.print();
}

TEST_F(PausePollerTest, WaitsWhenSpaceQueued) {
    std::ostringstream captured;
    std::streambuf* old_out = std::cout.rdbuf(captured.rdbuf());

    EXPECT_CALL(print_stats, print());
    EXPECT_CALL(is_tty, is_tty()).WillOnce(Return(true));
    EXPECT_CALL(try_read_byte, try_read_byte())
        .WillOnce(Return(std::optional<char>{' '}))
        .WillOnce(Return(std::optional<char>{' '}));
    EXPECT_CALL(idle, idle()).Times(1);

    poller.print();

    std::cout.rdbuf(old_out);
    EXPECT_THAT(captured.str(), HasSubstr("PAUSED"));
}

TEST_F(PausePollerTest, WaitIgnoresNonSpaceThenResumes) {
    std::ostringstream captured;
    std::streambuf* old_out = std::cout.rdbuf(captured.rdbuf());

    EXPECT_CALL(print_stats, print());
    EXPECT_CALL(is_tty, is_tty()).WillOnce(Return(true));
    EXPECT_CALL(try_read_byte, try_read_byte())
        .WillOnce(Return(std::optional<char>{' '}))
        .WillOnce(Return(std::optional<char>{'x'}))
        .WillOnce(Return(std::optional<char>{}))
        .WillOnce(Return(std::optional<char>{' '}));
    EXPECT_CALL(idle, idle()).Times(3);

    poller.print();

    std::cout.rdbuf(old_out);
    EXPECT_THAT(captured.str(), HasSubstr("PAUSED"));
}
