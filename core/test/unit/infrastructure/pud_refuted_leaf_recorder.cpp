#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_refuted_leaf_recorder.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMarkRefuted {
    MOCK_METHOD(void, mark_refuted, (pud_node_id));
};

struct MockCheckLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_node_id));
};

using test_recorder_t = pud_refuted_leaf_recorder<NiceMock<MockMarkRefuted>, NiceMock<MockCheckLeaf>>;

struct PudRefutedLeafRecorderTest : public ::testing::Test {
    NiceMock<MockMarkRefuted> mark_refuted;
    NiceMock<MockCheckLeaf> leaves;
    test_recorder_t recorder{mark_refuted, leaves};
    pud_node_id interior = 1;
    pud_node_id leaf_a = 2;
    pud_node_id leaf_b = 3;

    void SetUp() override {
        ON_CALL(leaves, check_leaf(leaf_a)).WillByDefault(Return(true));
        ON_CALL(leaves, check_leaf(leaf_b)).WillByDefault(Return(true));
    }
};

TEST_F(PudRefutedLeafRecorderTest, MarkingForwardsEveryNode) {
    EXPECT_CALL(mark_refuted, mark_refuted(interior));
    EXPECT_CALL(mark_refuted, mark_refuted(leaf_a));
    recorder.mark_refuted(interior);
    recorder.mark_refuted(leaf_a);
}

TEST_F(PudRefutedLeafRecorderTest, NothingIsRecordedBeforeMarking) {
    EXPECT_TRUE(recorder.take_refuted_leaves().empty());
}

TEST_F(PudRefutedLeafRecorderTest, MarkedLeavesAreRecordedInOrder) {
    recorder.mark_refuted(leaf_b);
    recorder.mark_refuted(leaf_a);
    EXPECT_THAT(recorder.take_refuted_leaves(), ElementsAre(leaf_b, leaf_a));
}

TEST_F(PudRefutedLeafRecorderTest, InteriorNodesAreNotRecorded) {
    recorder.mark_refuted(interior);
    recorder.mark_refuted(leaf_a);
    EXPECT_THAT(recorder.take_refuted_leaves(), ElementsAre(leaf_a));
}

TEST_F(PudRefutedLeafRecorderTest, TakingClearsTheRecord) {
    recorder.mark_refuted(leaf_a);
    recorder.take_refuted_leaves();
    EXPECT_TRUE(recorder.take_refuted_leaves().empty());
}

TEST_F(PudRefutedLeafRecorderTest, RecordingResumesAfterTaking) {
    recorder.mark_refuted(leaf_a);
    recorder.take_refuted_leaves();
    recorder.mark_refuted(leaf_b);
    EXPECT_THAT(recorder.take_refuted_leaves(), ElementsAre(leaf_b));
}

TEST_F(PudRefutedLeafRecorderTest, LeafIsCheckedAfterItIsMarked) {
    InSequence in_order;
    EXPECT_CALL(mark_refuted, mark_refuted(leaf_a));
    EXPECT_CALL(leaves, check_leaf(leaf_a)).WillOnce(Return(true));
    recorder.mark_refuted(leaf_a);
}

}  // namespace
