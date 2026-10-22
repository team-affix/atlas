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

struct MockMarkRoot {
    MOCK_METHOD(void, mark_root, (pud_node_id));
};

struct MockSetLeaf {
    MOCK_METHOD(void, set_leaf, (pud_node_id));
};

struct MockMarkAxiom {
    MOCK_METHOD(void, mark_axiom, (pud_node_id));
};

using test_initializer_t = pud_axiom_initializer<
    MockGetNextNodeID,
    MockStoreHead,
    MockStoreBodyGoals,
    MockStoreVarCount,
    MockMarkRoot,
    MockSetLeaf,
    MockMarkAxiom>;

struct PudAxiomInitializerTest : public ::testing::Test {
    MockGetNextNodeID get_next_id;
    MockStoreHead store_head;
    MockStoreBodyGoals store_body_goals;
    MockStoreVarCount store_var_count;
    MockMarkRoot mark_root;
    MockSetLeaf set_leaf;
    MockMarkAxiom mark_axiom;
    test_initializer_t initializer{
        get_next_id, store_head, store_body_goals, store_var_count, mark_root, set_leaf, mark_axiom};
    expr head{expr::var{0}};
    expr body_a{expr::var{1}};
    expr body_b{expr::var{2}};
};

TEST_F(PudAxiomInitializerTest, EmptyBodyStoresHeadAndMarksALeafRootAxiom) {
    EXPECT_CALL(get_next_id, next()).WillOnce(Return(pud_node_id{5}));
    EXPECT_CALL(store_head, store(pud_node_id{5}, &head));
    EXPECT_CALL(store_body_goals, store(pud_node_id{5}, std::vector<const expr*>{}));
    EXPECT_CALL(store_var_count, store(pud_node_id{5}, 4u));
    EXPECT_CALL(mark_root, mark_root(pud_node_id{5}));
    EXPECT_CALL(set_leaf, set_leaf(pud_node_id{5}));
    EXPECT_CALL(mark_axiom, mark_axiom(pud_node_id{5}));
    initializer.initialize_axiom(rule(&head, {}, 4));
}

TEST_F(PudAxiomInitializerTest, BodyGoalsStoredInOrder) {
    EXPECT_CALL(get_next_id, next()).WillOnce(Return(pud_node_id{7}));
    EXPECT_CALL(store_head, store(pud_node_id{7}, &head));
    EXPECT_CALL(store_body_goals, store(pud_node_id{7},
        std::vector<const expr*>{&body_a, &body_b}));
    EXPECT_CALL(store_var_count, store(pud_node_id{7}, 2u));
    EXPECT_CALL(mark_root, mark_root(pud_node_id{7}));
    EXPECT_CALL(set_leaf, set_leaf(pud_node_id{7}));
    EXPECT_CALL(mark_axiom, mark_axiom(pud_node_id{7}));
    initializer.initialize_axiom(rule(&head, {&body_a, &body_b}, 2));
}

TEST_F(PudAxiomInitializerTest, InitializeAxiomReturnsAssignedId) {
    EXPECT_CALL(get_next_id, next()).WillOnce(Return(pud_node_id{42}));
    EXPECT_CALL(store_head, store(_, _));
    EXPECT_CALL(store_body_goals, store(_, _));
    EXPECT_CALL(store_var_count, store(_, _));
    EXPECT_CALL(mark_root, mark_root(_));
    EXPECT_CALL(set_leaf, set_leaf(_));
    EXPECT_CALL(mark_axiom, mark_axiom(_));
    EXPECT_EQ(initializer.initialize_axiom(rule(&head, {}, 0)), pud_node_id{42});
}

TEST_F(PudAxiomInitializerTest, VarCountStoredExactly) {
    EXPECT_CALL(get_next_id, next()).WillOnce(Return(pud_node_id{1}));
    EXPECT_CALL(store_head, store(_, _));
    EXPECT_CALL(store_body_goals, store(_, _));
    EXPECT_CALL(store_var_count, store(pud_node_id{1}, 6u));
    EXPECT_CALL(mark_root, mark_root(_));
    EXPECT_CALL(set_leaf, set_leaf(_));
    EXPECT_CALL(mark_axiom, mark_axiom(_));
    initializer.initialize_axiom(rule(&head, {&body_a, &body_b}, 6));
}

TEST_F(PudAxiomInitializerTest, AxiomBecomesARootLeafAxiomAfterItsDataIsStored) {
    ::testing::InSequence in_order;
    EXPECT_CALL(get_next_id, next()).WillOnce(Return(pud_node_id{3}));
    EXPECT_CALL(store_head, store(pud_node_id{3}, _));
    EXPECT_CALL(store_body_goals, store(pud_node_id{3}, _));
    EXPECT_CALL(store_var_count, store(pud_node_id{3}, _));
    EXPECT_CALL(mark_root, mark_root(pud_node_id{3}));
    EXPECT_CALL(set_leaf, set_leaf(pud_node_id{3}));
    EXPECT_CALL(mark_axiom, mark_axiom(pud_node_id{3}));
    initializer.initialize_axiom(rule(&head, {}, 0));
}
