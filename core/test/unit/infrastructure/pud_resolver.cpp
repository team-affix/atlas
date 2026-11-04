#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_resolver.hpp"
#include "value_objects/pud_node_queries.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockRemoveQuery {
    MOCK_METHOD(void, remove, (pud_mhq_head_id));
};
using test_node_queries_t = pud_node_queries<NiceMock<MockRemoveQuery>>;
using test_owner_box_t = immer::box<pud_mhq_head_owner<NiceMock<MockRemoveQuery>>>;
struct MockGetNodeQueries {
    MOCK_METHOD(const test_node_queries_t&, get, (pud_global_node_id));
};
struct MockGetNodeBindings {
    MOCK_METHOD(const pud_ubs_state&, get, (pud_global_node_id));
};
struct MockGetNodeLvc {
    MOCK_METHOD(uint32_t, get, (pud_global_node_id));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id), (const));
};
struct MockGetQueryCandidates {
    MOCK_METHOD(std::vector<pud_mhcs_head_id>, candidates, (pud_mhq_head_id));
};
struct MockGetCandidatePath {
    MOCK_METHOD(const std::vector<pud_global_node_id>&, path, (pud_mhcs_head_id));
};
struct MockBuildInference {
    MOCK_METHOD(
        pud_inference,
        build,
        (pud_global_node_id,
         pud_body_goal_id,
         const pud_ubs_state&,
         uint32_t,
         const expr*,
         const std::vector<pud_global_node_id>&));
};

using test_resolver_t = pud_resolver<
    NiceMock<MockGetNodeQueries>, NiceMock<MockGetNodeBindings>, NiceMock<MockGetNodeLvc>, NiceMock<MockGetNodeGoals>, NiceMock<MockGetQueryCandidates>,
    NiceMock<MockGetCandidatePath>, NiceMock<MockBuildInference>>;

struct PudResolverTest : public ::testing::Test {
    NiceMock<MockRemoveQuery> remover;
    NiceMock<MockGetNodeQueries> node_queries;
    NiceMock<MockGetNodeBindings> node_bindings;
    NiceMock<MockGetNodeLvc> node_lvcs;
    NiceMock<MockGetNodeGoals> goals;
    NiceMock<MockGetQueryCandidates> candidates;
    NiceMock<MockGetCandidatePath> paths;
    NiceMock<MockBuildInference> inference_builder;
    test_resolver_t resolver{node_queries, node_bindings, node_lvcs, goals, candidates, paths, inference_builder};
    expr goal{expr::functor{1, {}}};
    pud_goal_list leaf_goals{.goals = immer::map<pud_body_goal_id, const expr*>{}.set(2, &goal), .next_id = 3};
    test_node_queries_t leaf_queries{test_node_queries_t{}.set(2, test_owner_box_t{remover, 40})};
    pud_ubs_state leaf_bindings;
    std::vector<pud_global_node_id> first_path{pud_test_node(0), pud_test_node(3)};
    std::vector<pud_global_node_id> second_path{pud_test_node(1)};
    pud_inference first_delta{.parent = 5, .call_site = 2, .substitutions = {}, .body_goals = {}, .var_count = 1};
    pud_inference second_delta{.parent = 5, .call_site = 2, .substitutions = {}, .body_goals = {}, .var_count = 2};

    void SetUp() override {
        ON_CALL(node_queries, get(pud_test_node(5))).WillByDefault(ReturnRef(leaf_queries));
        ON_CALL(node_bindings, get(pud_test_node(5))).WillByDefault(ReturnRef(leaf_bindings));
        ON_CALL(node_lvcs, get(pud_test_node(5))).WillByDefault(Return(8u));
        ON_CALL(goals, get(pud_test_node(5))).WillByDefault(ReturnRef(leaf_goals));
        ON_CALL(candidates, candidates(40)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{30, 31}));
        ON_CALL(paths, path(30)).WillByDefault(ReturnRef(first_path));
        ON_CALL(paths, path(31)).WillByDefault(ReturnRef(second_path));
    }
};

}

TEST_F(PudResolverTest, TheLeafsGoalIsAcceptedAgainstEveryCandidateInOrderAtTheLeafsLvc) {
    InSequence in_order;
    EXPECT_CALL(inference_builder, build(pud_test_node(5), 2u, leaf_bindings, 8u, &goal, first_path))
        .WillOnce(Return(first_delta));
    EXPECT_CALL(inference_builder, build(pud_test_node(5), 2u, leaf_bindings, 8u, &goal, second_path))
        .WillOnce(Return(second_delta));
    EXPECT_EQ(resolver.resolve(pud_test_node(5), 2), (std::vector<pud_inference>{first_delta, second_delta}));
}
