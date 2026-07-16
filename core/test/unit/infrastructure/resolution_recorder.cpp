// resolution_recorder: fans decision vs unit recording to decision and resolution memories.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/resolution_recorder.hpp"
#include "value_objects/lineage.hpp"

using ::testing::NiceMock;

namespace {

struct MockRecordDecision {
    MOCK_METHOD(void, record_decision, (const resolution_lineage*));
};

struct MockRecordResolution {
    MOCK_METHOD(void, record_resolution, (const resolution_lineage*));
};

using test_resolution_recorder_t = resolution_recorder<MockRecordDecision, MockRecordResolution>;

struct ResolutionRecorderTest : public ::testing::Test {
    NiceMock<MockRecordDecision> record_decision;
    NiceMock<MockRecordResolution> record_resolution;
    test_resolution_recorder_t recorder{record_decision, record_resolution};

    goal_lineage gl{nullptr, 0};
    resolution_lineage rl{&gl, 0};
};

TEST_F(ResolutionRecorderTest, DecisionResolutionNotifiesDecisionAndResolutionPaths) {
    EXPECT_CALL(record_decision, record_decision(&rl)).Times(1);
    EXPECT_CALL(record_resolution, record_resolution(&rl)).Times(1);

    recorder.record_decision_resolution(&rl);
}

TEST_F(ResolutionRecorderTest, UnitResolutionNotifiesResolutionPathOnly) {
    EXPECT_CALL(record_resolution, record_resolution(&rl)).Times(1);
    EXPECT_CALL(record_decision, record_decision(::testing::_)).Times(0);

    recorder.record_unit_resolution(&rl);
}

}  // namespace
