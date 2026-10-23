#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_node_indexer.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_node_delta.hpp"
#include "value_objects/rule.hpp"
#include "pud_test_node.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetAxiom {
    MOCK_METHOD(const rule&, get_axiom, (pud_global_node_id));
};
struct MockGetInference {
    MOCK_METHOD(const pud_node_delta&, get_inference, (pud_global_node_id));
};
struct MockGetGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id));
};
struct MockStoreGoals {
    MOCK_METHOD(void, store, (pud_global_node_id, pud_goal_list));
};
struct MockGetLvc {
    MOCK_METHOD(uint32_t, get, (pud_global_node_id));
};
struct MockStoreLvc {
    MOCK_METHOD(void, store, (pud_global_node_id, uint32_t));
};

using test_node_indexer_t = pud_node_indexer<
    NiceMock<MockGetAxiom>, NiceMock<MockGetInference>, NiceMock<MockGetGoals>, NiceMock<MockStoreGoals>,
    NiceMock<MockGetLvc>, NiceMock<MockStoreLvc>>;

// The axiom is node 2 of the test module, with body a, b, c and 3 variables. The inference is node 9
// of another module, resolved on goal 1 of its parent, node 4 of that module, which has goals a, b, c
// and lvc 5. The inference adds goals d and e and 2 variables.
struct PudNodeIndexerTest : public ::testing::Test {
    NiceMock<MockGetAxiom> axioms;
    NiceMock<MockGetInference> inferences;
    NiceMock<MockGetGoals> get_goals;
    NiceMock<MockStoreGoals> store_goals;
    NiceMock<MockGetLvc> get_lvc;
    NiceMock<MockStoreLvc> store_lvc;
    test_node_indexer_t indexer{axioms, inferences, get_goals, store_goals, get_lvc, store_lvc};
    expr head{expr::functor{1, {}}};
    expr goal_a{expr::functor{2, {}}};
    expr goal_b{expr::functor{3, {}}};
    expr goal_c{expr::functor{4, {}}};
    expr goal_d{expr::functor{5, {}}};
    expr goal_e{expr::functor{6, {}}};
    rule axiom{&head, {&goal_a, &goal_b, &goal_c}, 3};
    pud_module_id other_module = k_test_module + 1;
    pud_global_node_id inference{.module = other_module, .node = 9};
    pud_global_node_id parent{.module = other_module, .node = 4};
    pud_goal_list parent_goals{
        .goals = {
            pud_body_goal{.id = 0, .goal = &goal_a},
            pud_body_goal{.id = 1, .goal = &goal_b},
            pud_body_goal{.id = 2, .goal = &goal_c},
        },
        .next_id = 3,
    };
    pud_node_delta delta{
        .parent = 4, .call_site = 1, .substitutions = {}, .body_goals = {&goal_d, &goal_e}, .var_count = 2};

    void SetUp() override {
        ON_CALL(axioms, get_axiom(pud_test_node(2))).WillByDefault(ReturnRef(axiom));
        ON_CALL(inferences, get_inference(inference)).WillByDefault(ReturnRef(delta));
        ON_CALL(get_goals, get(parent)).WillByDefault(ReturnRef(parent_goals));
        ON_CALL(get_lvc, get(parent)).WillByDefault(Return(5));
    }
};

}

TEST_F(PudNodeIndexerTest, AnAxiomsGoalsAreItsBodyInOrderWithIdsFromZero) {
    EXPECT_CALL(store_goals, store(pud_test_node(2), (pud_goal_list{
        .goals = {
            pud_body_goal{.id = 0, .goal = &goal_a},
            pud_body_goal{.id = 1, .goal = &goal_b},
            pud_body_goal{.id = 2, .goal = &goal_c},
        },
        .next_id = 3,
    })));
    indexer.index_axiom(pud_test_node(2));
}

TEST_F(PudNodeIndexerTest, AnAxiomsLvcIsItsVariableCount) {
    EXPECT_CALL(store_lvc, store(pud_test_node(2), 3u));
    indexer.index_axiom(pud_test_node(2));
}

TEST_F(PudNodeIndexerTest, AnInferenceKeepsItsParentsGoalsButTheCallSiteUnderTheSameIdsThenAddsItsBodyWithFreshIds) {
    EXPECT_CALL(store_goals, store(inference, (pud_goal_list{
        .goals = {
            pud_body_goal{.id = 0, .goal = &goal_a},
            pud_body_goal{.id = 2, .goal = &goal_c},
            pud_body_goal{.id = 3, .goal = &goal_d},
            pud_body_goal{.id = 4, .goal = &goal_e},
        },
        .next_id = 5,
    })));
    indexer.index_inference(inference);
}

TEST_F(PudNodeIndexerTest, AnInferencesLvcIsItsParentsPlusItsVariableCount) {
    EXPECT_CALL(store_lvc, store(inference, 7u));
    indexer.index_inference(inference);
}

TEST_F(PudNodeIndexerTest, TheParentIsLookedUpInTheInferencesOwnModule) {
    EXPECT_CALL(get_goals, get(parent)).WillOnce(ReturnRef(parent_goals));
    EXPECT_CALL(get_lvc, get(parent)).WillOnce(Return(5));
    indexer.index_inference(inference);
}
