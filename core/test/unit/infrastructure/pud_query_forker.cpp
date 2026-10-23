#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_query_forker.hpp"
#include "value_objects/expr.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockForkQuery {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, try_fork, (pud_mhq_head_id, const pud_ubs_state&, const expr*, uint32_t));
};
struct MockFindNodeGoal {
    MOCK_METHOD(const expr*, find, (pud_global_node_id, pud_body_goal_id));
};
struct MockGetNodeLvc {
    MOCK_METHOD(uint32_t, get, (pud_global_node_id));
};

using test_query_forker_t =
    pud_query_forker<NiceMock<MockForkQuery>, NiceMock<MockFindNodeGoal>, NiceMock<MockGetNodeLvc>>;

struct PudQueryForkerTest : public ::testing::Test {
    NiceMock<MockForkQuery> mhq;
    NiceMock<MockFindNodeGoal> goals;
    NiceMock<MockGetNodeLvc> lvcs;
    test_query_forker_t forker{mhq, goals, lvcs};
    expr goal{expr::functor{1, {}}};
    pud_ubs_state bindings;

    void SetUp() override {
        bindings.parents = bindings.parents.set(5, 1);
        ON_CALL(goals, find(pud_test_node(2), 1)).WillByDefault(Return(&goal));
        ON_CALL(lvcs, get(pud_test_node(2))).WillByDefault(Return(6));
    }
};

}

TEST_F(PudQueryForkerTest, TheInheritedQueryIsForkedOnTheLeafsGoalInTheLeafsEnvironmentAtItsLvc) {
    EXPECT_CALL(mhq, try_fork(40u, bindings, &goal, 6u)).WillOnce(Return(60));
    EXPECT_EQ(forker.fork(40, pud_test_node(2), bindings, 1), std::optional<pud_mhq_head_id>{60});
}

TEST_F(PudQueryForkerTest, AForkWithoutCandidatesGetsNoQuery) {
    ON_CALL(mhq, try_fork(_, _, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_FALSE(forker.fork(40, pud_test_node(2), bindings, 1).has_value());
}
