#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_resolver.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetActiveState {
    MOCK_METHOD(const pud_active_state&, get, (pud_global_node_id));
};
struct MockFindNodeGoal {
    MOCK_METHOD(const expr*, find, (pud_global_node_id, pud_body_goal_id));
};
struct MockGetQueryCandidates {
    MOCK_METHOD(std::vector<pud_mhcs_head_id>, candidates, (pud_mhq_head_id));
};
struct MockGetCandidatePath {
    MOCK_METHOD(const std::vector<pud_global_node_id>&, path, (pud_mhcs_head_id));
};
struct MockAcceptCandidate {
    MOCK_METHOD(
        pud_inference,
        accept,
        (pud_global_node_id,
         pud_body_goal_id,
         const pud_ubs_state&,
         uint32_t,
         const expr*,
         const std::vector<pud_global_node_id>&));
};

using test_resolver_t = pud_resolver<
    NiceMock<MockGetActiveState>, NiceMock<MockFindNodeGoal>, NiceMock<MockGetQueryCandidates>,
    NiceMock<MockGetCandidatePath>, NiceMock<MockAcceptCandidate>>;

struct PudResolverTest : public ::testing::Test {
    NiceMock<MockGetActiveState> states;
    NiceMock<MockFindNodeGoal> goals;
    NiceMock<MockGetQueryCandidates> candidates;
    NiceMock<MockGetCandidatePath> paths;
    NiceMock<MockAcceptCandidate> acceptor;
    test_resolver_t resolver{states, goals, candidates, paths, acceptor};
    expr goal{expr::functor{1, {}}};
    pud_active_state state{
        .lvc = 8, .bindings = {}, .watch = {}, .queries = immer::map<pud_body_goal_id, pud_mhq_head_id>{}.set(2, 40)};
    std::vector<pud_global_node_id> first_path{pud_test_node(0), pud_test_node(3)};
    std::vector<pud_global_node_id> second_path{pud_test_node(1)};
    pud_inference first_delta{.parent = 5, .call_site = 2, .substitutions = {}, .body_goals = {}, .var_count = 1};
    pud_inference second_delta{.parent = 5, .call_site = 2, .substitutions = {}, .body_goals = {}, .var_count = 2};

    void SetUp() override {
        ON_CALL(states, get(pud_test_node(5))).WillByDefault(ReturnRef(state));
        ON_CALL(goals, find(pud_test_node(5), 2)).WillByDefault(Return(&goal));
        ON_CALL(candidates, candidates(40)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{30, 31}));
        ON_CALL(paths, path(30)).WillByDefault(ReturnRef(first_path));
        ON_CALL(paths, path(31)).WillByDefault(ReturnRef(second_path));
    }
};

}

TEST_F(PudResolverTest, TheLeafsGoalIsAcceptedAgainstEveryCandidateInOrderAtTheLeafsLvc) {
    InSequence in_order;
    EXPECT_CALL(acceptor, accept(pud_test_node(5), 2u, state.bindings, 8u, &goal, first_path))
        .WillOnce(Return(first_delta));
    EXPECT_CALL(acceptor, accept(pud_test_node(5), 2u, state.bindings, 8u, &goal, second_path))
        .WillOnce(Return(second_delta));
    EXPECT_EQ(resolver.resolve(pud_test_node(5), 2), (std::vector<pud_inference>{first_delta, second_delta}));
}
