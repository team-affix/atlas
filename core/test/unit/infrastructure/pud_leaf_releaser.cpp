#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_leaf_releaser.hpp"
#include "value_objects/pud_leaf_state.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetLeafState {
    MOCK_METHOD(const pud_leaf_state&, get, (pud_node_id));
};
struct MockEraseLeafState {
    MOCK_METHOD(void, erase, (pud_node_id));
};
struct MockReleaseQuery {
    MOCK_METHOD(bool, release, (pud_mhq_head_id, pud_node_id));
};
struct MockRemoveQuery {
    MOCK_METHOD(void, remove, (pud_mhq_head_id));
};

struct PudLeafReleaserTest : public ::testing::Test {
    NiceMock<MockGetLeafState> states;
    NiceMock<MockEraseLeafState> erase_states;
    NiceMock<MockReleaseQuery> holders;
    NiceMock<MockRemoveQuery> queries;
    pud_leaf_releaser<NiceMock<MockGetLeafState>, NiceMock<MockEraseLeafState>, NiceMock<MockReleaseQuery>, NiceMock<MockRemoveQuery>>
        releaser{states, erase_states, holders, queries};
    pud_leaf_state state{
        .bindings = {}, .watch = {}, .queries = immer::map<pud_body_goal_id, pud_mhq_head_id>{}.set(0, 30).set(1, 31)};

    void SetUp() override {
        ON_CALL(states, get(5)).WillByDefault(ReturnRef(state));
    }
};

}

TEST_F(PudLeafReleaserTest, TheLeafLetsGoOfEveryQueryItHolds) {
    EXPECT_CALL(holders, release(30u, 5u));
    EXPECT_CALL(holders, release(31u, 5u));
    releaser.release(5);
}

TEST_F(PudLeafReleaserTest, OnlyAQueryLeftWithoutHoldersIsRemoved) {
    ON_CALL(holders, release(30u, 5u)).WillByDefault(Return(true));
    ON_CALL(holders, release(31u, 5u)).WillByDefault(Return(false));
    EXPECT_CALL(queries, remove(30u));
    EXPECT_CALL(queries, remove(31u)).Times(0);
    releaser.release(5);
}

TEST_F(PudLeafReleaserTest, TheLeafsStateIsErased) {
    EXPECT_CALL(erase_states, erase(5));
    releaser.release(5);
}
