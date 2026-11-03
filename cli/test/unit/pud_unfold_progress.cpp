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
struct MockGetUnfoldDepthSum {
    MOCK_METHOD(size_t, unfold_depth_sum, (), (const));
};
struct MockCountNodes {
    MOCK_METHOD(size_t, count_nodes, (), (const));
};
struct MockCountFocusLeaves {
    MOCK_METHOD(pud_focus_leaf_counts, count_focus_leaves, (), (const));
};

using test_progress_t = pud_unfold_progress<
    NiceMock<MockGetTimeSinceStart>, NiceMock<MockGetUnfoldDepthSum>, NiceMock<MockCountNodes>,
    NiceMock<MockCountFocusLeaves>>;

// Captures std::cout. The clock reads 2s, the depth sum 20, the node count 50, and the focus has 8 live
// leaves, 2 of them blocked. Tests that print twice make the clock read 2s, then 4s, and the depth sum 20,
// then 26.
struct PudUnfoldProgressTest : public ::testing::Test {
    PudUnfoldProgressTest();
    ~PudUnfoldProgressTest() override;
    NiceMock<MockGetTimeSinceStart> timer;
    NiceMock<MockGetUnfoldDepthSum> depth_sum;
    NiceMock<MockCountNodes> nodes;
    NiceMock<MockCountFocusLeaves> focus_leaves;
    std::ostringstream captured_;
    std::streambuf* old_out_;
};

PudUnfoldProgressTest::PudUnfoldProgressTest()
    : timer()
    , depth_sum()
    , nodes()
    , focus_leaves()
    , captured_()
    , old_out_(std::cout.rdbuf(captured_.rdbuf())) {
    ON_CALL(timer, time_since_start()).WillByDefault(Return(std::chrono::seconds(2)));
    ON_CALL(depth_sum, unfold_depth_sum()).WillByDefault(Return(20));
    ON_CALL(nodes, count_nodes()).WillByDefault(Return(50));
    ON_CALL(focus_leaves, count_focus_leaves()).WillByDefault(Return(pud_focus_leaf_counts{.live = 8, .blocked = 2}));
}

PudUnfoldProgressTest::~PudUnfoldProgressTest() {
    std::cout.rdbuf(old_out_);
}

}

TEST_F(PudUnfoldProgressTest, EachLineShowsTotalsTheRateAndAverageDepthSinceTheLastLineAndElapsedTime) {
    EXPECT_CALL(timer, time_since_start())
        .WillOnce(Return(std::chrono::seconds(2)))
        .WillOnce(Return(std::chrono::seconds(4)));
    EXPECT_CALL(depth_sum, unfold_depth_sum()).WillOnce(Return(20)).WillOnce(Return(26));
    test_progress_t progress{timer, depth_sum, nodes, focus_leaves, false};
    for (int unfold = 0; unfold < 10; ++unfold)
        progress.on_unfold();
    progress.print();
    for (int unfold = 0; unfold < 6; ++unfold)
        progress.on_unfold();
    progress.print();
    EXPECT_EQ(
        captured_.str(),
        "10 unfolds | 5 unfolds/s | avg depth 2.0 | 50 nodes | focus leaves: 8 live, 2 blocked | TSS: 2.0s\n"
        "16 unfolds | 3 unfolds/s | avg depth 1.0 | 50 nodes | focus leaves: 8 live, 2 blocked | TSS: 4.0s\n");
}

TEST_F(PudUnfoldProgressTest, NothingIsPrintedWithoutNewUnfolds) {
    test_progress_t progress{timer, depth_sum, nodes, focus_leaves, true};
    progress.print();
    progress.finish_line();
    EXPECT_EQ(captured_.str(), "");
}

TEST_F(PudUnfoldProgressTest, OnATerminalEachLineOverwritesThePreviousOneInPlace) {
    EXPECT_CALL(timer, time_since_start())
        .WillOnce(Return(std::chrono::seconds(2)))
        .WillOnce(Return(std::chrono::seconds(4)));
    EXPECT_CALL(depth_sum, unfold_depth_sum()).WillOnce(Return(20)).WillOnce(Return(26));
    EXPECT_CALL(nodes, count_nodes()).WillOnce(Return(50)).WillOnce(Return(5));
    test_progress_t progress{timer, depth_sum, nodes, focus_leaves, true};
    for (int unfold = 0; unfold < 100; ++unfold)
        progress.on_unfold();
    progress.print();
    progress.on_unfold();
    progress.print();
    const std::string first =
        "100 unfolds | 50 unfolds/s | avg depth 0.2 | 50 nodes | focus leaves: 8 live, 2 blocked | TSS: 2.0s";
    const std::string second =
        "101 unfolds | 0 unfolds/s | avg depth 6.0 | 5 nodes | focus leaves: 8 live, 2 blocked | TSS: 4.0s";
    const std::string padding(first.size() - second.size(), ' ');
    EXPECT_EQ(captured_.str(), "\r" + first + "\033[K" + "\r" + second + padding + "\033[K");
}

TEST_F(PudUnfoldProgressTest, FinishingAnInPlaceLineEndsItOnce) {
    test_progress_t progress{timer, depth_sum, nodes, focus_leaves, true};
    progress.on_unfold();
    progress.print();
    progress.finish_line();
    progress.finish_line();
    EXPECT_EQ(
        captured_.str(),
        "\r1 unfolds | 0 unfolds/s | avg depth 20.0 | 50 nodes | focus leaves: 8 live, 2 blocked | TSS: 2.0s\033[K\n");
}
