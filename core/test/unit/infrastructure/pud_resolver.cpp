#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_resolver.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetLeafState {
    MOCK_METHOD(const pud_leaf_state&, get, (pud_node_id));
};
struct MockFindNodeGoal {
    MOCK_METHOD(const expr*, find, (pud_node_id, pud_body_goal_id));
};
struct MockGetNodeLvc {
    MOCK_METHOD(uint32_t, get, (pud_node_id));
};
struct MockGetQueryCandidates {
    MOCK_METHOD(std::vector<pud_mhcs_head_id>, candidates, (pud_mhq_head_id));
};
struct MockGetCandidatePath {
    MOCK_METHOD(const std::vector<pud_node_id>&, path, (pud_mhcs_head_id));
};
struct MockAcceptCandidate {
    MOCK_METHOD(pud_child_delta, accept, (const pud_ubs_state&, uint32_t, const expr*, const std::vector<pud_node_id>&));
};

using test_resolver_t = pud_resolver<
    NiceMock<MockGetLeafState>, NiceMock<MockFindNodeGoal>, NiceMock<MockGetNodeLvc>, NiceMock<MockGetQueryCandidates>,
    NiceMock<MockGetCandidatePath>, NiceMock<MockAcceptCandidate>>;

struct PudResolverTest : public ::testing::Test {
    NiceMock<MockGetLeafState> states;
    NiceMock<MockFindNodeGoal> goals;
    NiceMock<MockGetNodeLvc> lvcs;
    NiceMock<MockGetQueryCandidates> candidates;
    NiceMock<MockGetCandidatePath> paths;
    NiceMock<MockAcceptCandidate> acceptor;
    test_resolver_t resolver{states, goals, lvcs, candidates, paths, acceptor};
    expr goal{expr::functor{1, {}}};
    pud_leaf_state state{.bindings = {}, .watch = {}, .queries = immer::map<pud_body_goal_id, pud_mhq_head_id>{}.set(2, 40)};
    std::vector<pud_node_id> first_path{0, 3};
    std::vector<pud_node_id> second_path{1};
    pud_child_delta first_delta{.substitutions = {}, .body_goals = {}, .var_count = 1};
    pud_child_delta second_delta{.substitutions = {}, .body_goals = {}, .var_count = 2};

    void SetUp() override {
        ON_CALL(states, get(5)).WillByDefault(ReturnRef(state));
        ON_CALL(goals, find(5, 2)).WillByDefault(Return(&goal));
        ON_CALL(lvcs, get(5)).WillByDefault(Return(8));
        ON_CALL(candidates, candidates(40)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{30, 31}));
        ON_CALL(paths, path(30)).WillByDefault(ReturnRef(first_path));
        ON_CALL(paths, path(31)).WillByDefault(ReturnRef(second_path));
    }
};

}

TEST_F(PudResolverTest, TheGoalIsAcceptedAgainstEveryCandidateInOrder) {
    InSequence in_order;
    EXPECT_CALL(acceptor, accept(state.bindings, 8u, &goal, first_path)).WillOnce(Return(first_delta));
    EXPECT_CALL(acceptor, accept(state.bindings, 8u, &goal, second_path)).WillOnce(Return(second_delta));
    EXPECT_EQ(resolver.resolve(5, 2), (std::vector<pud_child_delta>{first_delta, second_delta}));
}
