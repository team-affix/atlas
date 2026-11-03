#include <chrono>
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unfold_progress.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetTimeSinceStart {
    MOCK_METHOD(std::chrono::nanoseconds, time_since_start, (), (const));
};

using test_progress_t = pud_unfold_progress<NiceMock<MockGetTimeSinceStart>>;

// Captures std::cout. The clock reads 2s; tests that print twice make it read 2s, then 4s.
struct PudUnfoldProgressTest : public ::testing::Test {
    PudUnfoldProgressTest();
    ~PudUnfoldProgressTest() override;
    NiceMock<MockGetTimeSinceStart> timer;
    std::ostringstream captured_;
    std::streambuf* old_out_;
};

PudUnfoldProgressTest::PudUnfoldProgressTest()
    : timer()
    , captured_()
    , old_out_(std::cout.rdbuf(captured_.rdbuf())) {
    ON_CALL(timer, time_since_start()).WillByDefault(Return(std::chrono::seconds(2)));
}

PudUnfoldProgressTest::~PudUnfoldProgressTest() {
    std::cout.rdbuf(old_out_);
}

}

TEST_F(PudUnfoldProgressTest, EachLineShowsTotalUnfoldsTheRateSinceTheLastLineAndElapsedTime) {
    EXPECT_CALL(timer, time_since_start())
        .WillOnce(Return(std::chrono::seconds(2)))
        .WillOnce(Return(std::chrono::seconds(4)));
    test_progress_t progress{timer, false};
    for (int unfold = 0; unfold < 10; ++unfold)
        progress.on_unfold();
    progress.print();
    for (int unfold = 0; unfold < 6; ++unfold)
        progress.on_unfold();
    progress.print();
    EXPECT_EQ(captured_.str(), "10 unfolds | 5 unfolds/s | TSS: 2.0s\n16 unfolds | 3 unfolds/s | TSS: 4.0s\n");
}

TEST_F(PudUnfoldProgressTest, NothingIsPrintedWithoutNewUnfolds) {
    test_progress_t progress{timer, true};
    progress.print();
    progress.finish_line();
    EXPECT_EQ(captured_.str(), "");
}

TEST_F(PudUnfoldProgressTest, OnATerminalEachLineOverwritesThePreviousOneInPlace) {
    EXPECT_CALL(timer, time_since_start())
        .WillOnce(Return(std::chrono::seconds(2)))
        .WillOnce(Return(std::chrono::seconds(4)));
    test_progress_t progress{timer, true};
    for (int unfold = 0; unfold < 100; ++unfold)
        progress.on_unfold();
    progress.print();
    progress.on_unfold();
    progress.print();
    const std::string first = "100 unfolds | 50 unfolds/s | TSS: 2.0s";
    const std::string second = "101 unfolds | 0 unfolds/s | TSS: 4.0s";
    const std::string padding(first.size() - second.size(), ' ');
    EXPECT_EQ(captured_.str(), "\r" + first + "\033[K" + "\r" + second + padding + "\033[K");
}

TEST_F(PudUnfoldProgressTest, FinishingAnInPlaceLineEndsItOnce) {
    test_progress_t progress{timer, true};
    progress.on_unfold();
    progress.print();
    progress.finish_line();
    progress.finish_line();
    EXPECT_EQ(captured_.str(), "\r1 unfolds | 0 unfolds/s | TSS: 2.0s\033[K\n");
}
