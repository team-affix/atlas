// dbuct_resolution_recorder: fans decision vs unit recording to the required bookkeeping oracles.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/dbuct_resolution_recorder.hpp"
#include "value_objects/lineage.hpp"

using ::testing::NiceMock;

namespace {

struct MockRecordDecision {
    MOCK_METHOD(void, record_decision, (const resolution_lineage*));
};

struct MockRecordResolution {
    MOCK_METHOD(void, record_resolution, (const resolution_lineage*));
};

struct MockNoteDecisionResolution {
    MOCK_METHOD(void, note_decision_resolution, (const resolution_lineage*));
};

struct MockNoteUnitResolution {
    MOCK_METHOD(void, note_unit_resolution, (const resolution_lineage*));
};

struct MockLogDecision {
    MOCK_METHOD(void, log_decision, (const resolution_lineage*));
};

using test_dbuct_resolution_recorder_t = dbuct_resolution_recorder<
    MockRecordDecision, MockRecordResolution,
    MockNoteDecisionResolution, MockNoteUnitResolution, MockLogDecision>;

struct DbuctResolutionRecorderTest : public ::testing::Test {
    NiceMock<MockRecordDecision> record_decision;
    NiceMock<MockRecordResolution> record_resolution;
    NiceMock<MockNoteDecisionResolution> note_decision;
    NiceMock<MockNoteUnitResolution> note_unit;
    NiceMock<MockLogDecision> log_decision;
    test_dbuct_resolution_recorder_t recorder{
        record_decision, record_resolution, note_decision, note_unit, log_decision};

    goal_lineage gl{nullptr, 0};
    resolution_lineage rl{&gl, 0};
};

TEST_F(DbuctResolutionRecorderTest, DecisionResolutionNotifiesDecisionAndResolutionPaths) {
    EXPECT_CALL(record_decision, record_decision(&rl)).Times(1);
    EXPECT_CALL(note_decision, note_decision_resolution(&rl)).Times(1);
    EXPECT_CALL(log_decision, log_decision(&rl)).Times(1);
    EXPECT_CALL(record_resolution, record_resolution(&rl)).Times(1);
    EXPECT_CALL(note_unit, note_unit_resolution(::testing::_)).Times(0);

    recorder.record_decision_resolution(&rl);
}

TEST_F(DbuctResolutionRecorderTest, UnitResolutionNotifiesResolutionPathOnly) {
    EXPECT_CALL(record_resolution, record_resolution(&rl)).Times(1);
    EXPECT_CALL(note_unit, note_unit_resolution(&rl)).Times(1);
    EXPECT_CALL(record_decision, record_decision(::testing::_)).Times(0);
    EXPECT_CALL(note_decision, note_decision_resolution(::testing::_)).Times(0);
    EXPECT_CALL(log_decision, log_decision(::testing::_)).Times(0);

    recorder.record_unit_resolution(&rl);
}

}  // namespace
