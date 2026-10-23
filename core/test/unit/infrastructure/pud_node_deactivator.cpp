#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_node_deactivator.hpp"
#include "value_objects/pud_active_state.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetActiveState {
    MOCK_METHOD(const pud_active_state&, get, (pud_node_id));
};
struct MockEraseActiveState {
    MOCK_METHOD(void, erase, (pud_node_id));
};
struct MockReleaseQuery {
    MOCK_METHOD(bool, release, (pud_mhq_head_id, pud_node_id));
};
struct MockRemoveQuery {
    MOCK_METHOD(void, remove, (pud_mhq_head_id));
};
struct MockInvalidateQueryLeaf {
    MOCK_METHOD(std::vector<pud_mhq_head_id>, invalidate_leaf, (pud_node_id));
};

using test_node_deactivator_t = pud_node_deactivator<
    NiceMock<MockGetActiveState>, NiceMock<MockEraseActiveState>, NiceMock<MockReleaseQuery>,
    NiceMock<MockRemoveQuery>, NiceMock<MockInvalidateQueryLeaf>>;

struct PudNodeDeactivatorTest : public ::testing::Test {
    NiceMock<MockGetActiveState> states;
    NiceMock<MockEraseActiveState> erase_states;
    NiceMock<MockReleaseQuery> holders;
    NiceMock<MockRemoveQuery> queries;
    NiceMock<MockInvalidateQueryLeaf> invalidate;
    test_node_deactivator_t deactivator{states, erase_states, holders, queries, invalidate};
    pud_active_state state{
        .bindings = {}, .watch = {}, .queries = immer::map<pud_body_goal_id, pud_mhq_head_id>{}.set(0, 30).set(1, 31)};

    void SetUp() override {
        ON_CALL(states, get(5)).WillByDefault(ReturnRef(state));
        ON_CALL(invalidate, invalidate_leaf(5)).WillByDefault(Return(std::vector<pud_mhq_head_id>{}));
    }
};

}

TEST_F(PudNodeDeactivatorTest, TheNodeLetsGoOfEveryQueryItHolds) {
    EXPECT_CALL(holders, release(30u, 5u));
    EXPECT_CALL(holders, release(31u, 5u));
    deactivator.deactivate(5);
}

TEST_F(PudNodeDeactivatorTest, OnlyAQueryLeftWithoutHoldersIsRemoved) {
    ON_CALL(holders, release(30u, 5u)).WillByDefault(Return(true));
    ON_CALL(holders, release(31u, 5u)).WillByDefault(Return(false));
    EXPECT_CALL(queries, remove(30u));
    EXPECT_CALL(queries, remove(31u)).Times(0);
    deactivator.deactivate(5);
}

TEST_F(PudNodeDeactivatorTest, TheNodesActiveStateIsErasedAndQueriesAreInvalidated) {
    EXPECT_CALL(erase_states, erase(5));
    EXPECT_CALL(invalidate, invalidate_leaf(5u)).WillOnce(Return(std::vector<pud_mhq_head_id>{40}));
    EXPECT_THAT(deactivator.deactivate(5), ElementsAre(40u));
}
