// lp_decision_recorder wraps the plain recorder and drives frame descent then
// entry. Its whole contract is the order: record, descend, enter.

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

struct MockEnterDecisionFrame {
    MOCK_METHOD(void, enter, (), ());
};

} // namespace

using test_lp_decision_recorder_t =
    lp_decision_recorder<StrictMock<MockRecordDecisionResolution>,
                         StrictMock<MockDescendToChildDecisionFrame>,
                         StrictMock<MockEnterDecisionFrame>>;

struct LpDecisionRecorderTest : public ::testing::Test {
protected:
    StrictMock<MockRecordDecisionResolution> record;
    StrictMock<MockDescendToChildDecisionFrame> descend;
    StrictMock<MockEnterDecisionFrame> enter;
    test_lp_decision_recorder_t recorder{record, descend, enter};

    goal_lineage g0{nullptr, 0};
    resolution_lineage g0_r0{&g0, 0};
    resolution_lineage g0_r1{&g0, 1};
};

TEST_F(LpDecisionRecorderTest, RecordsThenDescendsThenEnters) {
    InSequence seq;
    EXPECT_CALL(record, record_decision_resolution(&g0_r0));
    EXPECT_CALL(descend, descend(&g0_r0));
    EXPECT_CALL(enter, enter());

    recorder.record_decision_resolution(&g0_r0);
}

TEST_F(LpDecisionRecorderTest, EachDecisionDrivesItsOwnDescentAndEntry) {
    InSequence seq;
    EXPECT_CALL(record, record_decision_resolution(&g0_r0));
    EXPECT_CALL(descend, descend(&g0_r0));
    EXPECT_CALL(enter, enter());
    EXPECT_CALL(record, record_decision_resolution(&g0_r1));
    EXPECT_CALL(descend, descend(&g0_r1));
    EXPECT_CALL(enter, enter());

    recorder.record_decision_resolution(&g0_r0);
    recorder.record_decision_resolution(&g0_r1);
}
