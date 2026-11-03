#include <chrono>
#include <iostream>
#include <sstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_load_readout.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockNow {
    using time_point = std::chrono::steady_clock::time_point;
    MOCK_METHOD(time_point, now, (), (const));
};

using test_readout_t = pud_load_readout<NiceMock<MockNow>>;

// Captures std::cout. Tests that print make the clock read 1s, then 1.6s, then 3s.
struct PudLoadReadoutTest : public ::testing::Test {
    PudLoadReadoutTest();
    ~PudLoadReadoutTest() override;
    void expect_clock_reads();
    NiceMock<MockNow> clock;
    std::ostringstream captured_;
    std::streambuf* old_out_;
};

PudLoadReadoutTest::PudLoadReadoutTest()
    : clock()
    , captured_()
    , old_out_(std::cout.rdbuf(captured_.rdbuf())) {
    ON_CALL(clock, now()).WillByDefault(Return(MockNow::time_point{}));
}

void PudLoadReadoutTest::expect_clock_reads() {
    const MockNow::time_point start{};
    EXPECT_CALL(clock, now())
        .WillOnce(Return(start + std::chrono::milliseconds(1000)))
        .WillOnce(Return(start + std::chrono::milliseconds(1600)))
        .WillRepeatedly(Return(start + std::chrono::milliseconds(3000)));
}

PudLoadReadoutTest::~PudLoadReadoutTest() {
    std::cout.rdbuf(old_out_);
}

}

TEST_F(PudLoadReadoutTest, AStepIsAnnouncedThenReportedWithHowLongItTook) {
    expect_clock_reads();
    test_readout_t readout{clock, false};
    const MockNow::time_point started = readout.begin("Loading module 2a");
    readout.end("Module loaded", started);
    EXPECT_EQ(captured_.str(), "Loading module 2a...\nModule loaded in 600ms\n");
}

TEST_F(PudLoadReadoutTest, StepsNestByTheirOwnStartTimes) {
    expect_clock_reads();
    test_readout_t readout{clock, false};
    const MockNow::time_point outer = readout.begin("Initializing runtime");
    const MockNow::time_point inner = readout.begin("Loading module 2a");
    readout.end("Module loaded", inner);
    readout.end("Runtime initialized", outer);
    EXPECT_EQ(
        captured_.str(),
        "Initializing runtime...\nLoading module 2a...\nModule loaded in 1400ms\nRuntime initialized in 2000ms\n");
}

TEST_F(PudLoadReadoutTest, QuietPrintsNothing) {
    test_readout_t readout{clock, true};
    const MockNow::time_point started = readout.begin("Loading module 2a");
    readout.end("Module loaded", started);
    EXPECT_EQ(captured_.str(), "");
}
