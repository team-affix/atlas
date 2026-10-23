#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_query_opener.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_axiom_group_id.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockOpenQuery {
    MOCK_METHOD(std::optional<pud_mhq_head_id>, try_open, (pud_axiom_group_id, const pud_ubs_state&, const expr*, uint32_t));
};
struct MockFindNodeGoal {
    MOCK_METHOD(const expr*, find, (pud_node_id, pud_body_goal_id));
};
struct MockGetNodeLvc {
    MOCK_METHOD(uint32_t, get, (pud_node_id));
};
struct MockGetNodeAxiomGroup {
    MOCK_METHOD(pud_axiom_group_id, get, (pud_node_id));
};

using test_query_opener_t = pud_query_opener<
    NiceMock<MockOpenQuery>, NiceMock<MockFindNodeGoal>, NiceMock<MockGetNodeLvc>, NiceMock<MockGetNodeAxiomGroup>>;

struct PudQueryOpenerTest : public ::testing::Test {
    NiceMock<MockOpenQuery> mhq;
    NiceMock<MockFindNodeGoal> goals;
    NiceMock<MockGetNodeLvc> lvcs;
    NiceMock<MockGetNodeAxiomGroup> groups;
    test_query_opener_t opener{mhq, goals, lvcs, groups};
    pud_axiom_group_id group{.value = 3};
    expr goal{expr::functor{1, {}}};
    pud_ubs_state bindings;

    void SetUp() override {
        bindings.parents = bindings.parents.set(5, 1);
        ON_CALL(goals, find(2, 1)).WillByDefault(Return(&goal));
        ON_CALL(lvcs, get(2)).WillByDefault(Return(6));
        ON_CALL(groups, get(2)).WillByDefault(Return(group));
    }
};

}

TEST_F(PudQueryOpenerTest, TheGoalIsOpenedOverItsLeafsGroupInTheLeafsEnvironmentAtItsLvc) {
    EXPECT_CALL(mhq, try_open(group, bindings, &goal, 6u)).WillOnce(Return(50));
    EXPECT_EQ(opener.open(2, bindings, 1), std::optional<pud_mhq_head_id>{50});
}

TEST_F(PudQueryOpenerTest, AGoalWithoutCandidatesGetsNoQuery) {
    ON_CALL(mhq, try_open(_, _, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_FALSE(opener.open(2, bindings, 1).has_value());
}
