#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_axiom_initializer.hpp"
#include "value_objects/rule.hpp"

using ::testing::_;
using ::testing::Return;

struct MockGetNextNodeID {
    MOCK_METHOD(pud_node_id, next, ());
};

struct MockStoreHead {
    MOCK_METHOD(void, store, (pud_node_id, const expr*));
};

struct MockStoreBodyGoals {
    MOCK_METHOD(void, store, (pud_node_id, std::vector<const expr*>));
};

struct MockStoreVarCount {
    MOCK_METHOD(void, store, (pud_node_id, uint32_t));
};

struct MockRegisterRoot {
    MOCK_METHOD(void, register_root, (pud_node_id));
};

using test_initializer_t = pud_axiom_initializer<
    MockGetNextNodeID,
    MockStoreHead,
    MockStoreBodyGoals,
    MockStoreVarCount,
    MockRegisterRoot>;

struct PudAxiomInitializerTest : public ::testing::Test {
    MockGetNextNodeID get_next_id;
    MockStoreHead store_head;
    MockStoreBodyGoals store_body_goals;
    MockStoreVarCount store_var_count;
    MockRegisterRoot register_root;
    test_initializer_t initializer{
        get_next_id, store_head, store_body_goals, store_var_count, register_root};
    expr head{expr::var{0}};
    expr body_a{expr::var{1}};
    expr body_b{expr::var{2}};
};

TEST_F(PudAxiomInitializerTest, EmptyBodyStoresHeadAndRegistersRoot) {
    EXPECT_CALL(get_next_id, next()).WillOnce(Return(pud_node_id{5}));
    EXPECT_CALL(store_head, store(pud_node_id{5}, &head));
    EXPECT_CALL(store_body_goals, store(pud_node_id{5}, std::vector<const expr*>{}));
    EXPECT_CALL(store_var_count, store(pud_node_id{5}, 4u));
    EXPECT_CALL(register_root, register_root(pud_node_id{5}));
    initializer.initialize_axiom(rule(&head, {}, 4));
}

TEST_F(PudAxiomInitializerTest, BodyGoalsStoredInOrder) {
    EXPECT_CALL(get_next_id, next()).WillOnce(Return(pud_node_id{7}));
    EXPECT_CALL(store_head, store(pud_node_id{7}, &head));
    EXPECT_CALL(store_body_goals, store(pud_node_id{7},
        std::vector<const expr*>{&body_a, &body_b}));
    EXPECT_CALL(store_var_count, store(pud_node_id{7}, 2u));
    EXPECT_CALL(register_root, register_root(pud_node_id{7}));
    initializer.initialize_axiom(rule(&head, {&body_a, &body_b}, 2));
}

TEST_F(PudAxiomInitializerTest, InitializeAxiomReturnsAssignedId) {
    EXPECT_CALL(get_next_id, next()).WillOnce(Return(pud_node_id{42}));
    EXPECT_CALL(store_head, store(_, _));
    EXPECT_CALL(store_body_goals, store(_, _));
    EXPECT_CALL(store_var_count, store(_, _));
    EXPECT_CALL(register_root, register_root(_));
    EXPECT_EQ(initializer.initialize_axiom(rule(&head, {}, 0)), pud_node_id{42});
}

TEST_F(PudAxiomInitializerTest, VarCountStoredExactly) {
    EXPECT_CALL(get_next_id, next()).WillOnce(Return(pud_node_id{1}));
    EXPECT_CALL(store_head, store(_, _));
    EXPECT_CALL(store_body_goals, store(_, _));
    EXPECT_CALL(store_var_count, store(pud_node_id{1}, 6u));
    EXPECT_CALL(register_root, register_root(_));
    initializer.initialize_axiom(rule(&head, {&body_a, &body_b}, 6));
}
