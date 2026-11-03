#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unfold_depth_recorder.hpp"
#include "pud_test_node.hpp"

using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetNodeDepth {
    MOCK_METHOD(size_t, get, (pud_global_node_id), (const));
};
struct MockUnfoldLeaf {
    MOCK_METHOD(pud_unfold_outcome, unfold, (pud_global_node_id, pud_body_goal_id));
};

struct PudUnfoldDepthRecorderTest : public ::testing::Test {
    PudUnfoldDepthRecorderTest();
    NiceMock<MockGetNodeDepth> depths;
    NiceMock<MockUnfoldLeaf> unfolder;
    pud_unfold_depth_recorder<NiceMock<MockGetNodeDepth>, NiceMock<MockUnfoldLeaf>> recorder{depths, unfolder};
};

PudUnfoldDepthRecorderTest::PudUnfoldDepthRecorderTest() {
    ON_CALL(depths, get(pud_test_node(1))).WillByDefault(Return(3));
    ON_CALL(depths, get(pud_test_node(2))).WillByDefault(Return(5));
}

}

TEST_F(PudUnfoldDepthRecorderTest, TheSumStartsAtZero) {
    EXPECT_EQ(recorder.depth_sum(), 0u);
}

TEST_F(PudUnfoldDepthRecorderTest, EachUnfoldAddsTheDepthOfItsLeaf) {
    recorder.unfold(pud_test_node(1), 0);
    recorder.unfold(pud_test_node(2), 1);
    recorder.unfold(pud_test_node(1), 0);
    EXPECT_EQ(recorder.depth_sum(), 11u);
}

TEST_F(PudUnfoldDepthRecorderTest, TheDepthIsReadBeforeTheUnfoldIsForwarded) {
    InSequence order;
    EXPECT_CALL(depths, get(pud_test_node(2))).WillOnce(Return(5));
    EXPECT_CALL(unfolder, unfold(pud_test_node(2), 4u)).WillOnce(Return(pud_unfold_outcome::on_focus));
    recorder.unfold(pud_test_node(2), 4);
}

TEST_F(PudUnfoldDepthRecorderTest, TheUnfoldOutcomeIsPassedBack) {
    ON_CALL(unfolder, unfold(pud_test_node(1), 0u)).WillByDefault(Return(pud_unfold_outcome::off_focus));
    EXPECT_EQ(recorder.unfold(pud_test_node(1), 0), pud_unfold_outcome::off_focus);
}
