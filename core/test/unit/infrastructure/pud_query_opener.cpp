#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_query_opener.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_module_id.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockOpenQuery {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, try_open, (pud_module_id, const pud_ubs_state&, const expr*, uint32_t));
};
struct MockFindNodeGoal {
    MOCK_METHOD(const expr*, find, (pud_global_node_id, pud_body_goal_id));
};
struct MockGetNodeLvc {
    MOCK_METHOD(uint32_t, get, (pud_global_node_id));
};

using test_query_opener_t = pud_query_opener<NiceMock<MockOpenQuery>, NiceMock<MockFindNodeGoal>, NiceMock<MockGetNodeLvc>>;

struct PudQueryOpenerTest : public ::testing::Test {
    NiceMock<MockOpenQuery> mhq;
    NiceMock<MockFindNodeGoal> goals;
    NiceMock<MockGetNodeLvc> lvcs;
    test_query_opener_t opener{mhq, goals, lvcs};
    expr goal{expr::functor{1, {}}};
    pud_ubs_state bindings;

    void SetUp() override {
        bindings.parents = bindings.parents.set(5, 1);
        ON_CALL(goals, find(pud_test_node(2), 1)).WillByDefault(Return(&goal));
        ON_CALL(lvcs, get(pud_test_node(2))).WillByDefault(Return(6));
    }
};

}

TEST_F(PudQueryOpenerTest, TheGoalIsOpenedOverItsLeafsModuleInTheLeafsEnvironmentAtItsLvc) {
    EXPECT_CALL(mhq, try_open(k_test_module, bindings, &goal, 6u)).WillOnce(Return(50));
    EXPECT_EQ(opener.open(pud_test_node(2), bindings, 1), std::optional<pud_mhq_head_id>{50});
}

TEST_F(PudQueryOpenerTest, TheModuleOpenedOverIsTheLeafsOwn) {
    const pud_module_id other_module = k_test_module + 1;
    const pud_global_node_id leaf{.module = other_module, .node = 2};
    ON_CALL(goals, find(leaf, 1)).WillByDefault(Return(&goal));
    ON_CALL(lvcs, get(leaf)).WillByDefault(Return(6));
    EXPECT_CALL(mhq, try_open(other_module, bindings, &goal, 6u)).WillOnce(Return(50));
    EXPECT_EQ(opener.open(leaf, bindings, 1), std::optional<pud_mhq_head_id>{50});
}

TEST_F(PudQueryOpenerTest, AGoalWithoutCandidatesGetsNoQuery) {
    ON_CALL(mhq, try_open(_, _, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_FALSE(opener.open(pud_test_node(2), bindings, 1).has_value());
}
