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

struct PudUnfoldProgressTest : public ::testing::Test {
    PudUnfoldProgressTest();
    ~PudUnfoldProgressTest() override;
    NiceMock<MockGetTimeSinceStart> timer;
    pud_unfold_progress<NiceMock<MockGetTimeSinceStart>> progress;
    std::ostringstream captured_;
    std::streambuf* old_out_;
};

PudUnfoldProgressTest::PudUnfoldProgressTest()
    : timer()
    , progress(timer)
    , captured_()
    , old_out_(std::cout.rdbuf(captured_.rdbuf())) {
    ON_CALL(timer, time_since_start()).WillByDefault(Return(std::chrono::seconds(2)));
}

PudUnfoldProgressTest::~PudUnfoldProgressTest() {
    std::cout.rdbuf(old_out_);
}

}

TEST_F(PudUnfoldProgressTest, TheLineShowsTotalUnfoldsAndElapsedTime) {
    progress.on_unfold();
    progress.on_unfold();
    progress.print();
    progress.on_unfold();
    progress.print();
    EXPECT_EQ(captured_.str(), "2 unfolds | TSS: 2s\n3 unfolds | TSS: 2s\n");
}

TEST_F(PudUnfoldProgressTest, NothingIsPrintedWithoutNewUnfolds) {
    progress.print();
    EXPECT_EQ(captured_.str(), "");
}
