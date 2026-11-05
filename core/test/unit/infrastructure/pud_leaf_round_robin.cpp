#include <optional>
#include <set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_leaf_round_robin.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Optional;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetLiveLeaves {
    MOCK_METHOD(const std::set<pud_global_node_id>&, live, (), (const));
};
struct MockIsUnfoldable {
    MOCK_METHOD(bool, is_unfoldable, (pud_global_node_id), (const));
};

struct PudLeafRoundRobinTest : public ::testing::Test {
    NiceMock<MockGetLiveLeaves> live;
    NiceMock<MockIsUnfoldable> unfoldable;
    pud_leaf_round_robin<NiceMock<MockGetLiveLeaves>, NiceMock<MockIsUnfoldable>> round_robin{live, unfoldable};
    std::set<pud_global_node_id> leaves{pud_test_node(1), pud_test_node(2), pud_test_node(3)};

    void SetUp() override {
        ON_CALL(live, live()).WillByDefault(ReturnRef(leaves));
        ON_CALL(unfoldable, is_unfoldable(_)).WillByDefault(Return(true));
    }
};

}

TEST_F(PudLeafRoundRobinTest, LeavesAreDealtInIdOrderAndWrapAround) {
    EXPECT_THAT(round_robin.next(), Optional(pud_test_node(1)));
    EXPECT_THAT(round_robin.next(), Optional(pud_test_node(2)));
    EXPECT_THAT(round_robin.next(), Optional(pud_test_node(3)));
    EXPECT_THAT(round_robin.next(), Optional(pud_test_node(1)));
}

TEST_F(PudLeafRoundRobinTest, LeavesThatCannotBeUnfoldedArePassedOver) {
    ON_CALL(unfoldable, is_unfoldable(pud_test_node(2))).WillByDefault(Return(false));
    EXPECT_THAT(round_robin.next(), Optional(pud_test_node(1)));
    EXPECT_THAT(round_robin.next(), Optional(pud_test_node(3)));
}

TEST_F(PudLeafRoundRobinTest, DealingResumesAfterTheLastLeafDealtEvenOnceItLeavesTheFrontier) {
    EXPECT_THAT(round_robin.next(), Optional(pud_test_node(1)));
    leaves = {pud_test_node(0), pud_test_node(3), pud_test_node(4)};
    EXPECT_THAT(round_robin.next(), Optional(pud_test_node(3)));
    EXPECT_THAT(round_robin.next(), Optional(pud_test_node(4)));
    EXPECT_THAT(round_robin.next(), Optional(pud_test_node(0)));
}

TEST_F(PudLeafRoundRobinTest, NothingIsDealtWhenNoLeafCanBeUnfolded) {
    ON_CALL(unfoldable, is_unfoldable(_)).WillByDefault(Return(false));
    EXPECT_FALSE(round_robin.next().has_value());
}

TEST_F(PudLeafRoundRobinTest, NothingIsDealtFromAnEmptyFrontier) {
    leaves.clear();
    EXPECT_FALSE(round_robin.next().has_value());
}
