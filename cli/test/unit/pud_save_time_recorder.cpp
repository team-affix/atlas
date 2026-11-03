#include <chrono>
#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_save_time_recorder.hpp"

using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockNow {
    using time_point = std::chrono::steady_clock::time_point;
    using duration = std::chrono::steady_clock::duration;
    MOCK_METHOD(time_point, now, (), (const));
};
struct MockSaveAllModules {
    MOCK_METHOD(void, save_all, ());
};

struct PudSaveTimeRecorderTest : public ::testing::Test {
    NiceMock<MockNow> clock;
    NiceMock<MockSaveAllModules> modules;
    pud_save_time_recorder<NiceMock<MockNow>, NiceMock<MockSaveAllModules>> recorder{clock, modules};
    MockNow::time_point start{};
};

}

TEST_F(PudSaveTimeRecorderTest, BeforeTheFirstSaveThereIsNoTime) {
    EXPECT_FALSE(recorder.time_since_last_save().has_value());
}

TEST_F(PudSaveTimeRecorderTest, TheSaveIsForwardedBeforeItsTimeIsTaken) {
    InSequence order;
    EXPECT_CALL(modules, save_all());
    EXPECT_CALL(clock, now()).WillOnce(Return(start));
    recorder.save_all();
}

TEST_F(PudSaveTimeRecorderTest, TheTimeIsMeasuredFromTheLatestSave) {
    EXPECT_CALL(clock, now())
        .WillOnce(Return(start + std::chrono::seconds(10)))
        .WillOnce(Return(start + std::chrono::seconds(30)))
        .WillOnce(Return(start + std::chrono::seconds(80)));
    recorder.save_all();
    recorder.save_all();
    EXPECT_EQ(recorder.time_since_last_save(), std::optional<MockNow::duration>(std::chrono::seconds(50)));
}
