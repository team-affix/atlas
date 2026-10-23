#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_refuted_node_checker.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockCheckNodeLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_node_id), (const));
};
struct MockCheckLeafRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_node_id), (const));
};
struct MockGetLiveChildCount {
    MOCK_METHOD(size_t, get, (pud_node_id), (const));
};

struct PudRefutedNodeCheckerTest : public ::testing::Test {
    NiceMock<MockCheckNodeLeaf> leaves;
    NiceMock<MockCheckLeafRefuted> refuted_leaves;
    NiceMock<MockGetLiveChildCount> live_child_counts;
    pud_refuted_node_checker<NiceMock<MockCheckNodeLeaf>, NiceMock<MockCheckLeafRefuted>, NiceMock<MockGetLiveChildCount>>
        checker{leaves, refuted_leaves, live_child_counts};
};

}

TEST_F(PudRefutedNodeCheckerTest, ALiveLeafIsNotRefutedUnlessMarked) {
    ON_CALL(leaves, check_leaf(1)).WillByDefault(Return(true));
    ON_CALL(refuted_leaves, check_refuted(1)).WillByDefault(Return(false));
    EXPECT_FALSE(checker.check_refuted(1));
}

TEST_F(PudRefutedNodeCheckerTest, ARefutedLeafIsStillALeafAndRefuted) {
    ON_CALL(leaves, check_leaf(1)).WillByDefault(Return(true));
    ON_CALL(refuted_leaves, check_refuted(1)).WillByDefault(Return(true));
    EXPECT_TRUE(checker.check_refuted(1));
}

TEST_F(PudRefutedNodeCheckerTest, AnInteriorNodeIsRefutedExactlyWhenNoChildIsAlive) {
    ON_CALL(leaves, check_leaf(1)).WillByDefault(Return(false));
    ON_CALL(live_child_counts, get(1)).WillByDefault(Return(2));
    EXPECT_FALSE(checker.check_refuted(1));
    ON_CALL(live_child_counts, get(1)).WillByDefault(Return(0));
    EXPECT_TRUE(checker.check_refuted(1));
}

TEST_F(PudRefutedNodeCheckerTest, ALeafsChildCountIsNeverRead) {
    ON_CALL(leaves, check_leaf(1)).WillByDefault(Return(true));
    EXPECT_CALL(live_child_counts, get(::testing::_)).Times(0);
    checker.check_refuted(1);
}
