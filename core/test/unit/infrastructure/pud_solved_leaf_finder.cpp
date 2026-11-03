#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_solved_leaf_finder.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

pud_global_node_id node(pud_module_id module, pud_node_id id) {
    return pud_global_node_id{.module = module, .node = id};
}

struct MockGetSolvedFocusLeaves {
    MOCK_METHOD(const std::set<pud_global_node_id>&, solved, (), (const));
};

struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id), (const));
};

struct PudSolvedLeafFinderTest : public ::testing::Test {
    PudSolvedLeafFinderTest();
    NiceMock<MockGetSolvedFocusLeaves> focus_leaves;
    NiceMock<MockIsNodeRefuted> refuted;
    pud_solved_leaf_finder<NiceMock<MockGetSolvedFocusLeaves>, NiceMock<MockIsNodeRefuted>>
        finder{focus_leaves, refuted};
    std::set<pud_global_node_id> solved;

    void SetUp() override {
        ON_CALL(focus_leaves, solved()).WillByDefault(ReturnRef(solved));
        ON_CALL(refuted, is_refuted(_)).WillByDefault(Return(false));
    }
};

PudSolvedLeafFinderTest::PudSolvedLeafFinderTest()
    : focus_leaves()
    , refuted()
    , solved{node(1, 5), node(1, 1), node(2, 0)} {}

}

TEST_F(PudSolvedLeafFinderTest, NoSolvedFocusLeafMeansNoSolvedLeaf) {
    solved.clear();
    EXPECT_FALSE(finder.find().has_value());
}

TEST_F(PudSolvedLeafFinderTest, TheSmallestSolvedLeafIsReturned) {
    EXPECT_EQ(finder.find(), node(1, 1));
}

TEST_F(PudSolvedLeafFinderTest, ARefutedSolvedLeafIsPassedOver) {
    ON_CALL(refuted, is_refuted(node(1, 1))).WillByDefault(Return(true));
    EXPECT_EQ(finder.find(), node(1, 5));
}

TEST_F(PudSolvedLeafFinderTest, ListingReturnsEveryUnrefutedSolvedLeafInOrder) {
    ON_CALL(refuted, is_refuted(node(1, 5))).WillByDefault(Return(true));
    EXPECT_EQ(finder.list(), (std::vector<pud_global_node_id>{node(1, 1), node(2, 0)}));
}
