#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_node_maker.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetNextNodeId {
    MOCK_METHOD(pud_node_id, next, ());
};
struct MockStoreRootHead {
    MOCK_METHOD(void, store, (pud_node_id, const expr*));
};
struct MockStoreParent {
    MOCK_METHOD(void, store, (pud_node_id, std::optional<pud_node_id>));
};
struct MockStoreSubstitutions {
    MOCK_METHOD(void, store, (pud_node_id, std::vector<pud_substitution>));
};
struct MockGetGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_node_id));
};
struct MockStoreGoals {
    MOCK_METHOD(void, store, (pud_node_id, pud_goal_list));
};
struct MockGetLvc {
    MOCK_METHOD(uint32_t, get, (pud_node_id));
};
struct MockStoreLvc {
    MOCK_METHOD(void, store, (pud_node_id, uint32_t));
};
struct MockMarkNodeState {
    MOCK_METHOD(void, set_leaf, (pud_node_id));
};

using test_node_maker_t = pud_node_maker<
    NiceMock<MockGetNextNodeId>, NiceMock<MockStoreRootHead>, NiceMock<MockStoreParent>, NiceMock<MockStoreSubstitutions>,
    NiceMock<MockGetGoals>, NiceMock<MockStoreGoals>, NiceMock<MockGetLvc>, NiceMock<MockStoreLvc>, NiceMock<MockMarkNodeState>>;

struct PudNodeMakerTest : public ::testing::Test {
    NiceMock<MockGetNextNodeId> ids;
    NiceMock<MockStoreRootHead> root_heads;
    NiceMock<MockStoreParent> parents;
    NiceMock<MockStoreSubstitutions> substitutions;
    NiceMock<MockGetGoals> get_goals;
    NiceMock<MockStoreGoals> store_goals;
    NiceMock<MockGetLvc> get_lvc;
    NiceMock<MockStoreLvc> store_lvc;
    NiceMock<MockMarkNodeState> states;
    test_node_maker_t maker{ids, root_heads, parents, substitutions, get_goals, store_goals, get_lvc, store_lvc, states};
    expr head{expr::functor{1, {}}};
    expr goal_a{expr::functor{2, {}}};
    expr goal_b{expr::functor{3, {}}};
    expr goal_c{expr::functor{4, {}}};
    expr value{expr::functor{5, {}}};
    pud_goal_list parent_goals = pud_goal_list{.goals = {}, .next_id = 0}.adding(&goal_a).adding(&goal_b);

    void SetUp() override {
        ON_CALL(ids, next()).WillByDefault(Return(9));
        ON_CALL(get_goals, get(4)).WillByDefault(ReturnRef(parent_goals));
        ON_CALL(get_lvc, get(4)).WillByDefault(Return(5));
    }
};

}

TEST_F(PudNodeMakerTest, ARootIsALeafWithoutAParentHoldingItsHeadGoalsAndVariables) {
    InSequence in_order;
    EXPECT_CALL(states, set_leaf(9));
    EXPECT_CALL(parents, store(9, std::optional<pud_node_id>{}));
    EXPECT_CALL(root_heads, store(9, &head));
    EXPECT_CALL(store_goals, store(9, pud_goal_list{.goals = {}, .next_id = 0}.adding(&goal_a).adding(&goal_b)));
    EXPECT_CALL(store_lvc, store(9, 3u));
    EXPECT_EQ(maker.make_root(&head, {&goal_a, &goal_b}, 3), 9u);
}

TEST_F(PudNodeMakerTest, AChildClosesItsParentsCallSiteAndAppendsItsOwnGoals) {
    EXPECT_CALL(store_goals, store(9, parent_goals.closing(0).adding(&goal_c)));
    maker.make_child(4, 0, pud_child_delta{.substitutions = {}, .body_goals = {&goal_c}, .var_count = 0});
}

TEST_F(PudNodeMakerTest, AChildRecordsItsParentSubstitutionsAndFreshVariables) {
    const std::vector<pud_substitution> added{pud_substitution{.var_idx = 1, .value = &value}};
    EXPECT_CALL(states, set_leaf(9));
    EXPECT_CALL(parents, store(9, std::optional<pud_node_id>{4}));
    EXPECT_CALL(substitutions, store(9, added));
    EXPECT_CALL(store_lvc, store(9, 7u));
    EXPECT_CALL(root_heads, store(_, _)).Times(0);
    maker.make_child(4, 1, pud_child_delta{.substitutions = added, .body_goals = {}, .var_count = 2});
}
