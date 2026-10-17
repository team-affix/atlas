#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_conjecture_initializer.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockGetNextNodeID {
    MOCK_METHOD(pud_node_id, next, ());
};

struct MockStoreBodyGoals {
    MOCK_METHOD(void, store, (pud_node_id, std::vector<const expr*>));
};

struct MockStoreVarCount {
    MOCK_METHOD(void, store, (pud_node_id, uint32_t));
};

struct MockMarkRoot {
    MOCK_METHOD(void, mark_root, (pud_node_id));
};

struct MockSetLeaf {
    MOCK_METHOD(void, set_leaf, (pud_node_id));
};

struct MockDescent {
    int tag;
};

struct MockMakeRootDescent {
    MOCK_METHOD(MockDescent, make, (pud_node_id));
};

struct MockActivateLeaf {
    MOCK_METHOD(void, activate, (pud_node_id, MockDescent));
};

using test_initializer_t = pud_conjecture_initializer<
    StrictMock<MockGetNextNodeID>,
    StrictMock<MockStoreBodyGoals>,
    StrictMock<MockStoreVarCount>,
    StrictMock<MockMarkRoot>,
    StrictMock<MockSetLeaf>,
    StrictMock<MockMakeRootDescent>,
    StrictMock<MockActivateLeaf>>;

struct PudConjectureInitializerTest : public ::testing::Test {
    StrictMock<MockGetNextNodeID> get_next_id;
    StrictMock<MockStoreBodyGoals> store_body_goals;
    StrictMock<MockStoreVarCount> store_var_count;
    StrictMock<MockMarkRoot> mark_root;
    StrictMock<MockSetLeaf> set_leaf;
    StrictMock<MockMakeRootDescent> make_root_descent;
    StrictMock<MockActivateLeaf> activate_leaf;
    test_initializer_t initializer{
        get_next_id, store_body_goals, store_var_count, mark_root, set_leaf, make_root_descent, activate_leaf};
    expr goal_a{expr::var{0}};
    expr goal_b{expr::var{1}};
};

}  // namespace

TEST_F(PudConjectureInitializerTest, GoalsAndVarCountAreStoredOnARootLeaf) {
    EXPECT_CALL(get_next_id, next()).WillOnce(Return(pud_node_id{4}));
    EXPECT_CALL(store_body_goals, store(pud_node_id{4}, std::vector<const expr*>{&goal_a, &goal_b}));
    EXPECT_CALL(store_var_count, store(pud_node_id{4}, 3u));
    EXPECT_CALL(mark_root, mark_root(pud_node_id{4}));
    EXPECT_CALL(set_leaf, set_leaf(pud_node_id{4}));
    EXPECT_CALL(make_root_descent, make(pud_node_id{4})).WillOnce(Return(MockDescent{40}));
    EXPECT_CALL(activate_leaf, activate(pud_node_id{4}, _));
    initializer.initialize_conjecture({&goal_a, &goal_b}, 3);
}

TEST_F(PudConjectureInitializerTest, InitializeConjectureReturnsAssignedId) {
    EXPECT_CALL(get_next_id, next()).WillOnce(Return(pud_node_id{42}));
    EXPECT_CALL(store_body_goals, store(_, _));
    EXPECT_CALL(store_var_count, store(_, _));
    EXPECT_CALL(mark_root, mark_root(_));
    EXPECT_CALL(set_leaf, set_leaf(_));
    EXPECT_CALL(make_root_descent, make(_)).WillOnce(Return(MockDescent{0}));
    EXPECT_CALL(activate_leaf, activate(_, _));
    EXPECT_EQ(initializer.initialize_conjecture({}, 0), pud_node_id{42});
}

TEST_F(PudConjectureInitializerTest, ConjectureIsActivatedWithItsRootDescentAfterItsDataIsStored) {
    InSequence in_order;
    EXPECT_CALL(get_next_id, next()).WillOnce(Return(pud_node_id{3}));
    EXPECT_CALL(store_body_goals, store(pud_node_id{3}, _));
    EXPECT_CALL(store_var_count, store(pud_node_id{3}, _));
    EXPECT_CALL(mark_root, mark_root(pud_node_id{3}));
    EXPECT_CALL(set_leaf, set_leaf(pud_node_id{3}));
    EXPECT_CALL(make_root_descent, make(pud_node_id{3})).WillOnce(Return(MockDescent{30}));
    EXPECT_CALL(activate_leaf, activate(pud_node_id{3}, ::testing::Field(&MockDescent::tag, 30)));
    initializer.initialize_conjecture({&goal_a}, 1);
}
