#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_candidate_trail_resumer.hpp"
#include "pud_test_node.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_candidate_trail.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_specialization_state.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/pud_witness_search_stack.hpp"

using ::testing::_;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Optional;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;

namespace {

struct MockEnterReplay {
    MOCK_METHOD(
        std::optional<pud_replay>,
        enter,
        (const pud_ubs_state&, const expr*, uint32_t, pud_global_node_id, pud_module_id));
};
struct MockDescendReplay {
    MOCK_METHOD(std::optional<pud_replay>, descend, (const pud_replay&, pud_global_node_id));
};
struct MockFindAncestorPath {
    MOCK_METHOD(std::vector<pud_global_node_id>, find_path_from_root, (pud_global_node_id), (const));
    MOCK_METHOD(std::vector<pud_global_node_id>, find_path_below, (pud_global_node_id, pud_global_node_id), (const));
};
struct MockRebuildWitnessSearch {
    MOCK_METHOD(
        pud_witness_search_stack, rebuild, (const pud_replay&, (const std::vector<pud_global_node_id>&)), (const));
};
struct MockResumeWitnessSearch {
    MOCK_METHOD(std::optional<pud_global_node_id>, resume, (const pud_replay&, pud_witness_search_stack&));
};
struct MockSpecializeCandidate {
    MOCK_METHOD(bool, specialize, (pud_specialization_state&));
};
struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id));
};

pud_replay replay_at(pud_node_id node) {
    return pud_replay{.node = pud_test_node(node), .frame_offset = 0u, .ubs_state = {}};
}
std::optional<pud_replay> pud_enter_axiom(
    const pud_ubs_state&, const expr*, uint32_t, pud_global_node_id axiom, pud_module_id) {
    return pud_replay{.node = axiom, .frame_offset = 0u, .ubs_state = {}};
}
std::optional<pud_replay> pud_enter_child(const pud_replay& parent, pud_global_node_id child) {
    return pud_replay{.node = child, .frame_offset = parent.frame_offset, .ubs_state = parent.ubs_state};
}
// A rebuilt search stands on its witness leaf, so resuming it finds that leaf again.
std::optional<pud_global_node_id> pud_find_last_frame(const pud_replay& root, pud_witness_search_stack& search) {
    if (search.frames.empty())
        return root.node;
    return search.frames.back().current->node;
}
pud_witness_search_stack pud_stack_on(const pud_replay&, const std::vector<pud_global_node_id>& chain) {
    pud_witness_search_stack search{.frames = {}};
    for (const pud_global_node_id node : chain)
        search.frames.push_back(pud_witness_search_frame{
            .current = pud_replay{.node = node, .frame_offset = 0u, .ubs_state = {}}, .next_sibling = 1});
    return search;
}

// The specializer refilled witness a under child 5 after b survived under child 3.
bool pud_refill_after_survivor(pud_specialization_state& state) {
    const pud_replay under_three{.node = pud_test_node(3), .frame_offset = 0u, .ubs_state = {}};
    const pud_replay under_five{.node = pud_test_node(5), .frame_offset = 0u, .ubs_state = {}};
    state.witnesses->a = pud_specialization_witness{
        .search_root = under_five, .search = {.frames = {}}, .leaf = pud_test_node(5)};
    state.witnesses->b = pud_specialization_witness{
        .search_root = under_three, .search = {.frames = {}}, .leaf = pud_test_node(6)};
    return true;
}

using test_resumer_t = pud_candidate_trail_resumer<
    NiceMock<MockEnterReplay>, NiceMock<MockDescendReplay>, NiceMock<MockFindAncestorPath>,
    NiceMock<MockRebuildWitnessSearch>, NiceMock<MockResumeWitnessSearch>, NiceMock<MockSpecializeCandidate>,
    NiceMock<MockGetChildren>>;

// The axiom root 0 has children 1, 2; the candidate 2 has children 3, 4, 5; below 3 lies leaf 6. The trail's
// candidate is 2, witnessed by 6 (chain 3, 6) and 5 (chain 5).
struct PudCandidateTrailResumerTest : public ::testing::Test {
    NiceMock<MockEnterReplay> enter;
    NiceMock<MockDescendReplay> descend;
    NiceMock<MockFindAncestorPath> paths;
    NiceMock<MockRebuildWitnessSearch> rebuild;
    NiceMock<MockResumeWitnessSearch> resume;
    NiceMock<MockSpecializeCandidate> specialize;
    NiceMock<MockGetChildren> children;
    test_resumer_t resumer{enter, descend, paths, rebuild, resume, specialize, children};
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> child_lists{{0, {1, 2}}, {2, {3, 4, 5}}, {3, {6}}};
    pud_ubs_state bindings{};
    expr goal{.content = expr::var{.index = 0}};
    pud_candidate_trail trail{.candidate = pud_test_node(2), .witness_leaves = {pud_test_node(6), pud_test_node(5)}};
    pud_specialization_state specialized{.path = {}, .current = replay_at(0), .witnesses = std::nullopt};

    void SetUp() override {
        ON_CALL(enter, enter(_, _, _, _, _)).WillByDefault(Invoke(&pud_enter_axiom));
        ON_CALL(descend, descend(_, _)).WillByDefault(Invoke(&pud_enter_child));
        ON_CALL(paths, find_path_from_root(pud_test_node(2)))
            .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(0), pud_test_node(2)}));
        ON_CALL(paths, find_path_below(pud_test_node(2), pud_test_node(6)))
            .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(3), pud_test_node(6)}));
        ON_CALL(paths, find_path_below(pud_test_node(2), pud_test_node(5)))
            .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(5)}));
        ON_CALL(rebuild, rebuild(_, _)).WillByDefault(Invoke(&pud_stack_on));
        ON_CALL(resume, resume(_, _)).WillByDefault(Invoke(&pud_find_last_frame));
        ON_CALL(specialize, specialize(_)).WillByDefault(DoAll(SaveArg<0>(&specialized), Return(true)));
        for (const auto& [node, listed] : child_lists)
            ON_CALL(children, get_children(pud_test_node(node))).WillByDefault(ReturnRef(child_lists.at(node)));
    }
};

}

TEST_F(PudCandidateTrailResumerTest, TheAxiomRootIsEnteredUnderTheCallersBindings) {
    EXPECT_CALL(enter, enter(::testing::Ref(bindings), &goal, 4u, pud_test_node(0), k_test_module));
    resumer.resume(k_test_module, bindings, &goal, 4, trail);
}

TEST_F(PudCandidateTrailResumerTest, AnAxiomWhoseHeadNoLongerResolvesTheGoalLeavesNoCandidate) {
    ON_CALL(enter, enter(_, _, _, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(specialize, specialize(_)).Times(0);
    EXPECT_FALSE(resumer.resume(k_test_module, bindings, &goal, 4, trail).has_value());
}

TEST_F(PudCandidateTrailResumerTest, TheCandidateStandsWhereItWasWithBothWitnessesResumed) {
    resumer.resume(k_test_module, bindings, &goal, 4, trail);
    EXPECT_THAT(specialized.path, ElementsAre(pud_test_node(0), pud_test_node(2)));
    EXPECT_EQ(specialized.current.node, pud_test_node(2));
    ASSERT_TRUE(specialized.witnesses.has_value());
    EXPECT_EQ(specialized.witnesses->a->search_root.node, pud_test_node(3));
    EXPECT_EQ(specialized.witnesses->a->leaf, pud_test_node(6));
    EXPECT_EQ(specialized.witnesses->b->search_root.node, pud_test_node(5));
    EXPECT_EQ(specialized.witnesses->b->leaf, pud_test_node(5));
    EXPECT_EQ(specialized.witnesses->next_child, 3u);
}

TEST_F(PudCandidateTrailResumerTest, EachWitnessSearchIsRebuiltFromTheChainBelowItsRoot) {
    EXPECT_CALL(rebuild, rebuild(Field(&pud_replay::node, pud_test_node(3)), ElementsAre(pud_test_node(6))));
    EXPECT_CALL(rebuild, rebuild(Field(&pud_replay::node, pud_test_node(5)), IsEmpty()));
    resumer.resume(k_test_module, bindings, &goal, 4, trail);
}

TEST_F(PudCandidateTrailResumerTest, AWitnessTheSearchLosesIsLeftForTheSpecializerToRefill) {
    // The specializer refills what is missing; this test only reads the state it was handed.
    ON_CALL(specialize, specialize(_)).WillByDefault(DoAll(SaveArg<0>(&specialized), Return(false)));
    ON_CALL(resume, resume(Field(&pud_replay::node, pud_test_node(3)), _)).WillByDefault(Return(std::nullopt));
    resumer.resume(k_test_module, bindings, &goal, 4, trail);
    EXPECT_FALSE(specialized.witnesses->a.has_value());
    EXPECT_EQ(specialized.witnesses->b->leaf, pud_test_node(5));
}

TEST_F(PudCandidateTrailResumerTest, AWitnessRootThatNoLongerReplaysIsLost) {
    // The specializer refills what is missing; this test only reads the state it was handed.
    ON_CALL(specialize, specialize(_)).WillByDefault(DoAll(SaveArg<0>(&specialized), Return(false)));
    ON_CALL(descend, descend(_, pud_test_node(5))).WillByDefault(Return(std::nullopt));
    resumer.resume(k_test_module, bindings, &goal, 4, trail);
    EXPECT_FALSE(specialized.witnesses->b.has_value());
    EXPECT_EQ(specialized.witnesses->next_child, 3u);
}

TEST_F(PudCandidateTrailResumerTest, APathNodeThatNoLongerReplaysLeavesTheCandidateOnItsParentLookingAfterIt) {
    // The specializer refills what is missing; this test only reads the state it was handed.
    ON_CALL(specialize, specialize(_)).WillByDefault(DoAll(SaveArg<0>(&specialized), Return(false)));
    ON_CALL(descend, descend(_, pud_test_node(2))).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(rebuild, rebuild(_, _)).Times(0);
    resumer.resume(k_test_module, bindings, &goal, 4, trail);
    EXPECT_THAT(specialized.path, ElementsAre(pud_test_node(0)));
    EXPECT_EQ(specialized.current.node, pud_test_node(0));
    ASSERT_TRUE(specialized.witnesses.has_value());
    EXPECT_FALSE(specialized.witnesses->a.has_value());
    EXPECT_FALSE(specialized.witnesses->b.has_value());
    EXPECT_EQ(specialized.witnesses->next_child, 2u);
}

TEST_F(PudCandidateTrailResumerTest, ASelfWitnessedCandidateIsSpecializedWithoutWitnesses) {
    trail.witness_leaves = {pud_test_node(2)};
    EXPECT_CALL(rebuild, rebuild(_, _)).Times(0);
    resumer.resume(k_test_module, bindings, &goal, 4, trail);
    EXPECT_EQ(specialized.current.node, pud_test_node(2));
    EXPECT_FALSE(specialized.witnesses.has_value());
}

TEST_F(PudCandidateTrailResumerTest, TheResumedCandidateListsItsPathAndWitnessLeaves) {
    EXPECT_THAT(
        resumer.resume(k_test_module, bindings, &goal, 4, trail),
        Optional(pud_eager_candidate{
            .path = {pud_test_node(0), pud_test_node(2)}, .witness_leaves = {pud_test_node(6), pud_test_node(5)}}));
}

TEST_F(PudCandidateTrailResumerTest, AnUnjustifiedCandidateIsLeftOut) {
    ON_CALL(specialize, specialize(_)).WillByDefault(Return(false));
    EXPECT_FALSE(resumer.resume(k_test_module, bindings, &goal, 4, trail).has_value());
}

TEST_F(PudCandidateTrailResumerTest, WitnessLeavesAreListedInTheirChildOrderLikeAFreshQuery) {
    ON_CALL(specialize, specialize(_)).WillByDefault(Invoke(&pud_refill_after_survivor));
    EXPECT_THAT(
        resumer.resume(k_test_module, bindings, &goal, 4, trail),
        Optional(pud_eager_candidate{
            .path = {pud_test_node(0), pud_test_node(2)}, .witness_leaves = {pud_test_node(6), pud_test_node(5)}}));
}
