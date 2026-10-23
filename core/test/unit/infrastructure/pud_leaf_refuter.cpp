#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_leaf_refuter.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMarkLeafRefuted {
    MOCK_METHOD(void, mark_refuted, (pud_global_node_id));
};
struct MockPropagateRefutation {
    MOCK_METHOD(void, refute, (pud_global_node_id));
};
struct MockDeactivateNode {
    MOCK_METHOD(std::vector<pud_global_node_id>, deactivate, (pud_global_node_id));
};
struct MockCheckNodeLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_global_node_id));
};
struct MockCheckLeafRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_global_node_id));
};

using test_refuter_t = pud_leaf_refuter<
    NiceMock<MockMarkLeafRefuted>, NiceMock<MockPropagateRefutation>, NiceMock<MockDeactivateNode>,
    NiceMock<MockCheckNodeLeaf>, NiceMock<MockCheckLeafRefuted>>;

struct PudLeafRefuterTest : public ::testing::Test {
    NiceMock<MockMarkLeafRefuted> marks;
    NiceMock<MockPropagateRefutation> propagator;
    NiceMock<MockDeactivateNode> deactivator;
    NiceMock<MockCheckNodeLeaf> leaves;
    NiceMock<MockCheckLeafRefuted> refuted_leaves;
    test_refuter_t refuter{marks, propagator, deactivator, leaves, refuted_leaves};

    void record_refuted(pud_global_node_id leaf) {
        ON_CALL(refuted_leaves, check_refuted(leaf)).WillByDefault(Return(true));
    }

    void SetUp() override {
        ON_CALL(deactivator, deactivate(_)).WillByDefault(Return(std::vector<pud_global_node_id>{}));
        ON_CALL(leaves, check_leaf(_)).WillByDefault(Return(true));
        ON_CALL(refuted_leaves, check_refuted(_)).WillByDefault(Return(false));
    }
};

}

TEST_F(PudLeafRefuterTest, ALeafIsMarkedPropagatedAndDeactivatedInThatOrder) {
    InSequence in_order;
    EXPECT_CALL(marks, mark_refuted(pud_test_node(6)));
    EXPECT_CALL(propagator, refute(pud_test_node(6)));
    EXPECT_CALL(deactivator, deactivate(pud_test_node(6)));
    EXPECT_THAT(refuter.refute(pud_test_node(6)), ElementsAre(pud_test_node(6)));
}

TEST_F(PudLeafRefuterTest, EveryLeafTheDeactivationStrandsIsRefuted) {
    ON_CALL(deactivator, deactivate(pud_test_node(6))).WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(7), pud_test_node(8)}));
    EXPECT_CALL(marks, mark_refuted(pud_test_node(6)));
    EXPECT_CALL(marks, mark_refuted(pud_test_node(7)));
    EXPECT_CALL(marks, mark_refuted(pud_test_node(8)));
    EXPECT_THAT(refuter.refute(pud_test_node(6)), ElementsAre(pud_test_node(6), pud_test_node(7), pud_test_node(8)));
}

TEST_F(PudLeafRefuterTest, TheCascadeContinuesThroughEveryRefutedLeaf) {
    ON_CALL(deactivator, deactivate(pud_test_node(6))).WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(7)}));
    ON_CALL(deactivator, deactivate(pud_test_node(7))).WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(8)}));
    EXPECT_THAT(refuter.refute(pud_test_node(6)), ElementsAre(pud_test_node(6), pud_test_node(7), pud_test_node(8)));
}

TEST_F(PudLeafRefuterTest, AStrandedInteriorNodeIsLeftToItsChildren) {
    ON_CALL(deactivator, deactivate(pud_test_node(6))).WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(7)}));
    ON_CALL(leaves, check_leaf(pud_test_node(7))).WillByDefault(Return(false));
    EXPECT_CALL(marks, mark_refuted(pud_test_node(6)));
    EXPECT_CALL(deactivator, deactivate(pud_test_node(6)));
    EXPECT_THAT(refuter.refute(pud_test_node(6)), ElementsAre(pud_test_node(6)));
}

TEST_F(PudLeafRefuterTest, ALeafAlreadyRefutedIsLeftAlone) {
    ON_CALL(refuted_leaves, check_refuted(pud_test_node(6))).WillByDefault(Return(true));
    EXPECT_CALL(marks, mark_refuted(_)).Times(0);
    EXPECT_CALL(deactivator, deactivate(_)).Times(0);
    EXPECT_THAT(refuter.refute(pud_test_node(6)), IsEmpty());
}

TEST_F(PudLeafRefuterTest, ALeafStrandedTwiceIsRefutedOnce) {
    ON_CALL(deactivator, deactivate(pud_test_node(6))).WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(7), pud_test_node(7)}));
    EXPECT_CALL(marks, mark_refuted(pud_test_node(6)));
    EXPECT_CALL(marks, mark_refuted(pud_test_node(7))).WillOnce(Invoke(this, &PudLeafRefuterTest::record_refuted));
    EXPECT_THAT(refuter.refute(pud_test_node(6)), ElementsAre(pud_test_node(6), pud_test_node(7)));
}
