#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_child_maker.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetNextNodeId {
    MOCK_METHOD(pud_node_id, next, ());
};

struct MockStoreSubstitutions {
    MOCK_METHOD(void, store, (pud_node_id, std::vector<pud_substitution>));
};

struct MockStoreBodyGoals {
    MOCK_METHOD(void, store, (pud_node_id, std::vector<const expr*>));
};

struct MockStoreVarCount {
    MOCK_METHOD(void, store, (pud_node_id, uint32_t));
};

struct MockStoreParent {
    MOCK_METHOD(void, store, (pud_node_id, pud_node_id));
};

struct MockSetLeaf {
    MOCK_METHOD(void, set_leaf, (pud_node_id));
};

const expr* expr_at(uintptr_t address) {
    return reinterpret_cast<const expr*>(address);
}

struct PudChildMakerTest : public ::testing::Test {
    NiceMock<MockGetNextNodeId> next_id;
    NiceMock<MockStoreSubstitutions> substitutions;
    NiceMock<MockStoreBodyGoals> body_goals;
    NiceMock<MockStoreVarCount> var_counts;
    NiceMock<MockStoreParent> parents;
    NiceMock<MockSetLeaf> leaves;
    pud_child_maker<
        NiceMock<MockGetNextNodeId>, NiceMock<MockStoreSubstitutions>, NiceMock<MockStoreBodyGoals>,
        NiceMock<MockStoreVarCount>, NiceMock<MockStoreParent>, NiceMock<MockSetLeaf>>
        maker{next_id, substitutions, body_goals, var_counts, parents, leaves};
    pud_node_id parent = 4;
    pud_child_delta delta{
        .substitutions = {pud_substitution{.var_idx = 1, .value = expr_at(0x10)}},
        .body_goals = {expr_at(0x20), expr_at(0x30)},
        .var_count = 2,
    };

    void SetUp() override {
        ON_CALL(next_id, next()).WillByDefault(Return(9));
    }
};

TEST_F(PudChildMakerTest, ReturnsTheNextNodeId) {
    EXPECT_EQ(maker.make(parent, delta), 9u);
}

TEST_F(PudChildMakerTest, StoresTheDeltaUnderTheChild) {
    EXPECT_CALL(substitutions, store(9u, delta.substitutions));
    EXPECT_CALL(body_goals, store(9u, delta.body_goals));
    EXPECT_CALL(var_counts, store(9u, 2u));
    maker.make(parent, delta);
}

TEST_F(PudChildMakerTest, ChildIsALeafUnderItsParent) {
    EXPECT_CALL(parents, store(9u, parent));
    EXPECT_CALL(leaves, set_leaf(9u));
    maker.make(parent, delta);
}

TEST_F(PudChildMakerTest, ChildBecomesALeafOnlyAfterItsDataIsStored) {
    InSequence in_order;
    EXPECT_CALL(next_id, next()).WillOnce(Return(9));
    EXPECT_CALL(substitutions, store(9u, _));
    EXPECT_CALL(body_goals, store(9u, _));
    EXPECT_CALL(var_counts, store(9u, _));
    EXPECT_CALL(parents, store(9u, _));
    EXPECT_CALL(leaves, set_leaf(9u));
    maker.make(parent, delta);
}

TEST_F(PudChildMakerTest, EachChildGetsItsOwnId) {
    EXPECT_CALL(next_id, next()).WillOnce(Return(9)).WillOnce(Return(10));
    EXPECT_EQ(maker.make(parent, delta), 9u);
    EXPECT_EQ(maker.make(parent, delta), 10u);
}

}  // namespace
