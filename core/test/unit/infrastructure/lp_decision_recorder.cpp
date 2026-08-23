// lp_decision_recorder wraps the plain recorder and drives frame descent. Its
// whole contract is the order: record the decision, then descend.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/lp_decision_recorder.hpp"
#include "value_objects/lineage.hpp"

using ::testing::InSequence;
using ::testing::StrictMock;

namespace {

struct MockRecordDecisionResolution {
    MOCK_METHOD(void, record_decision_resolution, (const resolution_lineage*), ());
};

struct MockDescendToChildDecisionFrame {
    MOCK_METHOD(void, descend, (const resolution_lineage*), ());
};

} // namespace

using test_lp_decision_recorder_t =
    lp_decision_recorder<StrictMock<MockRecordDecisionResolution>,
                         StrictMock<MockDescendToChildDecisionFrame>>;

struct LpDecisionRecorderTest : public ::testing::Test {
protected:
    StrictMock<MockRecordDecisionResolution> record;
    StrictMock<MockDescendToChildDecisionFrame> descend;
    test_lp_decision_recorder_t recorder{record, descend};

    goal_lineage g0{nullptr, 0};
    resolution_lineage g0_r0{&g0, 0};
    resolution_lineage g0_r1{&g0, 1};
};

TEST_F(LpDecisionRecorderTest, RecordsThenDescends) {
    InSequence seq;
    EXPECT_CALL(record, record_decision_resolution(&g0_r0));
    EXPECT_CALL(descend, descend(&g0_r0));

    recorder.record_decision_resolution(&g0_r0);
}

TEST_F(LpDecisionRecorderTest, EachDecisionDrivesItsOwnDescent) {
    InSequence seq;
    EXPECT_CALL(record, record_decision_resolution(&g0_r0));
    EXPECT_CALL(descend, descend(&g0_r0));
    EXPECT_CALL(record, record_decision_resolution(&g0_r1));
    EXPECT_CALL(descend, descend(&g0_r1));

    recorder.record_decision_resolution(&g0_r0);
    recorder.record_decision_resolution(&g0_r1);
}
