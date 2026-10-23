#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_node_maker.hpp"
#include "value_objects/pud_axiom_group_id.hpp"
#include "value_objects/pud_node_delta.hpp"
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockStoreAxiom {
    MOCK_METHOD(pud_node_id, store_axiom, (const rule&));
};
struct MockStoreInference {
    MOCK_METHOD(pud_node_id, store_inference, (pud_node_delta));
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
struct MockGetNodeAxiomGroup {
    MOCK_METHOD(pud_axiom_group_id, get, (pud_node_id));
};
struct MockStoreNodeAxiomGroup {
    MOCK_METHOD(void, store, (pud_node_id, pud_axiom_group_id));
};
struct MockAddAxiomGroupMember {
    MOCK_METHOD(void, add_member, (pud_axiom_group_id, pud_node_id));
};

using test_node_maker_t = pud_node_maker<
    NiceMock<MockStoreAxiom>, NiceMock<MockStoreInference>, NiceMock<MockGetGoals>, NiceMock<MockStoreGoals>,
    NiceMock<MockGetLvc>, NiceMock<MockStoreLvc>, NiceMock<MockGetNodeAxiomGroup>, NiceMock<MockStoreNodeAxiomGroup>,
    NiceMock<MockAddAxiomGroupMember>>;

// Every node made is stored as node 9. The parent of an inference is node 4, with goals a and b and
// lvc 5.
struct PudNodeMakerTest : public ::testing::Test {
    NiceMock<MockStoreAxiom> axioms;
    NiceMock<MockStoreInference> inferences;
    NiceMock<MockGetGoals> get_goals;
    NiceMock<MockStoreGoals> store_goals;
    NiceMock<MockGetLvc> get_lvc;
    NiceMock<MockStoreLvc> store_lvc;
    NiceMock<MockGetNodeAxiomGroup> get_axiom_groups;
    NiceMock<MockStoreNodeAxiomGroup> store_axiom_groups;
    NiceMock<MockAddAxiomGroupMember> group_members;
    test_node_maker_t maker{
        axioms, inferences, get_goals, store_goals, get_lvc, store_lvc, get_axiom_groups, store_axiom_groups,
        group_members};
    pud_axiom_group_id group{.value = 1};
    expr head{expr::functor{1, {}}};
    expr goal_a{expr::functor{2, {}}};
    expr goal_b{expr::functor{3, {}}};
    expr goal_c{expr::functor{4, {}}};
    expr value{expr::functor{5, {}}};
    pud_goal_list parent_goals = pud_goal_list{.goals = {}, .next_id = 0}.adding(&goal_a).adding(&goal_b);

    void SetUp() override {
        ON_CALL(axioms, store_axiom(_)).WillByDefault(Return(9));
        ON_CALL(inferences, store_inference(_)).WillByDefault(Return(9));
        ON_CALL(get_goals, get(4)).WillByDefault(ReturnRef(parent_goals));
        ON_CALL(get_lvc, get(4)).WillByDefault(Return(5));
        ON_CALL(get_axiom_groups, get(4)).WillByDefault(Return(group));
    }
};

}

TEST_F(PudNodeMakerTest, ARootIsStoredAsItsAxiomWithItsGoalsAndVariables) {
    const rule axiom(&head, {&goal_a, &goal_b}, 3);
    EXPECT_CALL(axioms, store_axiom(axiom)).WillOnce(Return(9));
    EXPECT_CALL(store_goals, store(9, pud_goal_list{.goals = {}, .next_id = 0}.adding(&goal_a).adding(&goal_b)));
    EXPECT_CALL(store_lvc, store(9, 3u));
    EXPECT_EQ(maker.make_root(axiom, group), 9u);
}

TEST_F(PudNodeMakerTest, ARootBelongsToItsGroupAndIsAnAxiomOfIt) {
    EXPECT_CALL(store_axiom_groups, store(9, group));
    EXPECT_CALL(group_members, add_member(group, 9u));
    maker.make_root(rule(&head, {}, 0), group);
}

TEST_F(PudNodeMakerTest, AChildIsStoredAsItsInference) {
    const pud_node_delta delta{
        .parent = 4,
        .call_site = 1,
        .substitutions = {pud_substitution{.var_idx = 1, .value = &value}},
        .body_goals = {},
        .var_count = 2,
    };
    EXPECT_CALL(inferences, store_inference(delta)).WillOnce(Return(9));
    EXPECT_EQ(maker.make_child(delta), 9u);
}

TEST_F(PudNodeMakerTest, AChildClosesItsParentsCallSiteAndAppendsItsOwnGoals) {
    EXPECT_CALL(store_goals, store(9, parent_goals.closing(0).adding(&goal_c)));
    maker.make_child(pud_node_delta{.parent = 4, .call_site = 0, .substitutions = {}, .body_goals = {&goal_c}, .var_count = 0});
}

TEST_F(PudNodeMakerTest, AChildAddsItsFreshVariablesToItsParentsAndJoinsItsParentsGroup) {
    EXPECT_CALL(store_lvc, store(9, 7u));
    EXPECT_CALL(store_axiom_groups, store(9, group));
    EXPECT_CALL(group_members, add_member(_, _)).Times(0);
    maker.make_child(pud_node_delta{.parent = 4, .call_site = 1, .substitutions = {}, .body_goals = {}, .var_count = 2});
}
