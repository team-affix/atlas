#include <cstdint>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_trail_goal_querier.hpp"
#include "pud_test_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_candidate_trail.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_goal_trails.hpp"
#include "value_objects/pud_ubs_state.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockQueryGoal {
    MOCK_METHOD(
        std::vector<pud_eager_candidate>, query, (pud_module_id, const pud_ubs_state&, const expr*, uint32_t), (const));
};
struct MockResumeTrail {
    MOCK_METHOD(
        std::optional<pud_eager_candidate>,
        resume,
        (pud_module_id, const pud_ubs_state&, const expr*, uint32_t, const pud_candidate_trail&),
        (const));
};
struct MockFindAncestorPath {
    MOCK_METHOD(std::vector<pud_global_node_id>, find_path_from_root, (pud_global_node_id), (const));
};
struct MockIsNodeLeaf {
    MOCK_METHOD(bool, is_leaf, (pud_global_node_id), (const));
};
struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id), (const));
};

using test_querier_t = pud_trail_goal_querier<
    NiceMock<MockQueryGoal>, NiceMock<MockResumeTrail>, NiceMock<MockFindAncestorPath>, NiceMock<MockIsNodeLeaf>,
    NiceMock<MockIsNodeRefuted>>;

// Two trails: candidate 2 witnessed by 6 and 7, and candidate 3 witnessing itself.
struct PudTrailGoalQuerierTest : public ::testing::Test {
    NiceMock<MockQueryGoal> fresh;
    NiceMock<MockResumeTrail> resumer;
    NiceMock<MockFindAncestorPath> paths;
    NiceMock<MockIsNodeLeaf> leaves;
    NiceMock<MockIsNodeRefuted> refuted;
    test_querier_t querier{fresh, resumer, paths, leaves, refuted};
    pud_ubs_state bindings{};
    expr goal{.content = expr::var{.index = 0}};
    pud_candidate_trail first{.candidate = pud_test_node(2), .witness_leaves = {pud_test_node(6), pud_test_node(7)}};
    pud_candidate_trail second{.candidate = pud_test_node(3), .witness_leaves = {pud_test_node(3)}};
    pud_goal_trails trails{.candidates = {first, second}, .forked = false};
    pud_eager_candidate resumed{.path = {pud_test_node(0), pud_test_node(8)}, .witness_leaves = {pud_test_node(8)}};

    void SetUp() override {
        ON_CALL(leaves, is_leaf(_)).WillByDefault(Return(true));
        ON_CALL(refuted, is_refuted(_)).WillByDefault(Return(false));
        ON_CALL(paths, find_path_from_root(pud_test_node(2)))
            .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(0), pud_test_node(2)}));
        ON_CALL(paths, find_path_from_root(pud_test_node(3)))
            .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(1), pud_test_node(3)}));
        ON_CALL(resumer, resume(_, _, _, _, _)).WillByDefault(Return(resumed));
    }
};

}

TEST_F(PudTrailGoalQuerierTest, AGoalWithoutTrailsIsQueriedFresh) {
    const std::vector<pud_eager_candidate> found{resumed};
    EXPECT_CALL(fresh, query(k_test_module, ::testing::Ref(bindings), &goal, 4u)).WillOnce(Return(found));
    EXPECT_EQ(querier.query(k_test_module, bindings, &goal, 4, nullptr), found);
}

TEST_F(PudTrailGoalQuerierTest, UnforkedTrailsWithLiveWitnessesAreTheSameCandidatesWithoutAReplay) {
    EXPECT_CALL(resumer, resume(_, _, _, _, _)).Times(0);
    EXPECT_CALL(fresh, query(_, _, _, _)).Times(0);
    EXPECT_THAT(
        querier.query(k_test_module, bindings, &goal, 4, &trails),
        ElementsAre(
            pud_eager_candidate{
                .path = {pud_test_node(0), pud_test_node(2)}, .witness_leaves = {pud_test_node(6), pud_test_node(7)}},
            pud_eager_candidate{.path = {pud_test_node(1), pud_test_node(3)}, .witness_leaves = {pud_test_node(3)}}));
}

TEST_F(PudTrailGoalQuerierTest, ATrailWithAnUnfoldedWitnessIsResumedInItsPlace) {
    ON_CALL(leaves, is_leaf(pud_test_node(7))).WillByDefault(Return(false));
    EXPECT_CALL(resumer, resume(k_test_module, ::testing::Ref(bindings), &goal, 4u, first)).WillOnce(Return(resumed));
    EXPECT_THAT(
        querier.query(k_test_module, bindings, &goal, 4, &trails),
        ElementsAre(
            resumed,
            pud_eager_candidate{.path = {pud_test_node(1), pud_test_node(3)}, .witness_leaves = {pud_test_node(3)}}));
}

TEST_F(PudTrailGoalQuerierTest, ATrailWithARefutedWitnessIsResumed) {
    ON_CALL(refuted, is_refuted(pud_test_node(3))).WillByDefault(Return(true));
    EXPECT_CALL(resumer, resume(_, _, _, _, second)).WillOnce(Return(resumed));
    querier.query(k_test_module, bindings, &goal, 4, &trails);
}

TEST_F(PudTrailGoalQuerierTest, ForkedTrailsAreAllResumed) {
    trails.forked = true;
    EXPECT_CALL(resumer, resume(_, _, _, _, first)).WillOnce(Return(resumed));
    EXPECT_CALL(resumer, resume(_, _, _, _, second)).WillOnce(Return(resumed));
    EXPECT_THAT(querier.query(k_test_module, bindings, &goal, 4, &trails), ElementsAre(resumed, resumed));
}

TEST_F(PudTrailGoalQuerierTest, AResumedTrailThatIsNoLongerACandidateIsDropped) {
    trails.forked = true;
    ON_CALL(resumer, resume(_, _, _, _, first)).WillByDefault(Return(std::nullopt));
    EXPECT_THAT(querier.query(k_test_module, bindings, &goal, 4, &trails), ElementsAre(resumed));
}
