#include <chrono>
#include <sstream>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_tick_progress.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetTimeSinceStart {
    MOCK_METHOD(std::chrono::nanoseconds, time_since_start, (), (const));
};
struct MockGetUnfoldCount {
    MOCK_METHOD(size_t, unfold_count, (), (const));
};

struct PudTickProgressTest : public ::testing::Test {
    PudTickProgressTest();
    ~PudTickProgressTest() override;
    NiceMock<MockGetTimeSinceStart> timer;
    NiceMock<MockGetUnfoldCount> runtime;
    pud_tick_progress<NiceMock<MockGetTimeSinceStart>, NiceMock<MockGetUnfoldCount>> progress;
    std::ostringstream captured_;
    std::streambuf* old_out_;
};

PudTickProgressTest::PudTickProgressTest()
    : timer()
    , runtime()
    , progress(timer, runtime)
    , captured_()
    , old_out_(std::cout.rdbuf(captured_.rdbuf())) {
    ON_CALL(timer, time_since_start()).WillByDefault(Return(std::chrono::seconds(2)));
}

PudTickProgressTest::~PudTickProgressTest() {
    std::cout.rdbuf(old_out_);
}

}

TEST_F(PudTickProgressTest, TheLineShowsTicksUnfoldsAndElapsedTime) {
    ON_CALL(runtime, unfold_count()).WillByDefault(Return(17));
    progress.on_tick();
    progress.on_tick();
    progress.print();
    EXPECT_EQ(captured_.str(), "2 ticks | 17 unfolds | TSS: 2s\n");
}

TEST_F(PudTickProgressTest, NothingIsPrintedWithoutNewTicks) {
    progress.print();
    EXPECT_EQ(captured_.str(), "");
}
