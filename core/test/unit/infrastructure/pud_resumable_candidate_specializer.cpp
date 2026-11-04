#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_resumable_candidate_specializer.hpp"
#include "pud_test_node.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_replay.hpp"
#include "value_objects/pud_specialization_state.hpp"
#include "value_objects/pud_witness_advance.hpp"
#include "value_objects/pud_witness_search_stack.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockDescendReplay {
    MOCK_METHOD(std::optional<pud_replay>, descend, (const pud_replay&, pud_global_node_id));
};
struct MockResumeWitnessSearch {
    MOCK_METHOD(std::optional<pud_global_node_id>, resume, (const pud_replay&, pud_witness_search_stack&));
};
struct MockAdvanceWitnessSearch {
    MOCK_METHOD(std::optional<pud_witness_advance>, advance, (pud_witness_search_stack&));
};
struct MockIsNodeLeaf {
    MOCK_METHOD(bool, is_leaf, (pud_global_node_id), (const));
};
struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id), (const));
};
struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id));
};

pud_replay replay_at(pud_node_id node) {
    return pud_replay{.node = pud_test_node(node), .frame_offset = 0u, .ubs_state = {}};
}
std::optional<pud_replay> pud_enter_child(const pud_replay& parent, pud_global_node_id child) {
    return pud_replay{.node = child, .frame_offset = parent.frame_offset, .ubs_state = parent.ubs_state};
}
// A search root that is a leaf witnesses itself, with nothing left on its stack.
std::optional<pud_global_node_id> pud_root_witnesses_itself(const pud_replay& root, pud_witness_search_stack&) {
    return root.node;
}
// A stack the search left at depth one cannot advance past its root.
std::optional<pud_witness_advance> pud_cannot_advance(pud_witness_search_stack&) {
    return std::nullopt;
}

using test_specializer_t = pud_resumable_candidate_specializer<
    NiceMock<MockDescendReplay>,
    NiceMock<MockResumeWitnessSearch>,
    NiceMock<MockAdvanceWitnessSearch>,
    NiceMock<MockIsNodeLeaf>,
    NiceMock<MockIsNodeRefuted>,
    NiceMock<MockGetChildren>>;

struct PudResumableCandidateSpecializerTest : public ::testing::Test {
    NiceMock<MockDescendReplay> descend;
    NiceMock<MockResumeWitnessSearch> resume;
    NiceMock<MockAdvanceWitnessSearch> advance;
    NiceMock<MockIsNodeLeaf> leaves;
    NiceMock<MockIsNodeRefuted> refuted;
    NiceMock<MockGetChildren> children;
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> child_lists;
    test_specializer_t specializer{descend, resume, advance, leaves, refuted, children};
    pud_specialization_state state{.path = {pud_test_node(0)}, .current = replay_at(0), .witnesses = std::nullopt};

    void SetUp() override {
        ON_CALL(descend, descend(_, _)).WillByDefault(Invoke(&pud_enter_child));
        ON_CALL(resume, resume(_, _)).WillByDefault(Invoke(&pud_root_witnesses_itself));
        ON_CALL(advance, advance(_)).WillByDefault(Invoke(&pud_cannot_advance));
        ON_CALL(leaves, is_leaf(_)).WillByDefault(Return(false));
        ON_CALL(refuted, is_refuted(_)).WillByDefault(Return(false));
    }

    void interior(pud_node_id node, std::vector<pud_node_id> node_children) {
        child_lists[node] = std::move(node_children);
        ON_CALL(children, get_children(pud_test_node(node))).WillByDefault(ReturnRef(child_lists[node]));
    }
    void leaf(pud_node_id node) {
        interior(node, {});
        ON_CALL(leaves, is_leaf(pud_test_node(node))).WillByDefault(Return(true));
    }
};

}

TEST_F(PudResumableCandidateSpecializerTest, ALeafAxiomRootIsJustifiedAsItsOwnWitness) {
    leaf(0);
    EXPECT_TRUE(specializer.specialize(state));
    EXPECT_THAT(state.path, ElementsAre(pud_test_node(0)));
    EXPECT_FALSE(state.witnesses.has_value());
}

TEST_F(PudResumableCandidateSpecializerTest, ARefutedAxiomRootIsNotJustified) {
    leaf(0);
    ON_CALL(refuted, is_refuted(pud_test_node(0))).WillByDefault(Return(true));
    EXPECT_FALSE(specializer.specialize(state));
}

TEST_F(PudResumableCandidateSpecializerTest, WitnessesInTwoChildSubtreesKeepTheCandidateWhereItStands) {
    interior(0, {1, 2});
    EXPECT_TRUE(specializer.specialize(state));
    EXPECT_THAT(state.path, ElementsAre(pud_test_node(0)));
    ASSERT_TRUE(state.witnesses.has_value());
    EXPECT_EQ(state.witnesses->a->leaf, pud_test_node(1));
    EXPECT_EQ(state.witnesses->b->leaf, pud_test_node(2));
}

TEST_F(PudResumableCandidateSpecializerTest, ANodeWithNoWitnessingChildIsNotJustified) {
    interior(0, {1, 2});
    ON_CALL(resume, resume(_, _)).WillByDefault(Return(std::nullopt));
    EXPECT_FALSE(specializer.specialize(state));
}

TEST_F(PudResumableCandidateSpecializerTest, BlockedChildrenAreNotSearched) {
    interior(0, {1, 2, 3});
    ON_CALL(descend, descend(_, pud_test_node(2))).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(resume, resume(_, _)).Times(AnyNumber());
    EXPECT_CALL(resume, resume(Field(&pud_replay::node, pud_test_node(2)), _)).Times(0);
    EXPECT_TRUE(specializer.specialize(state));
    EXPECT_EQ(state.witnesses->b->leaf, pud_test_node(3));
}

TEST_F(PudResumableCandidateSpecializerTest, ASoleWitnessingChildThatIsALeafBecomesTheCandidate) {
    interior(0, {1, 2});
    ON_CALL(descend, descend(_, pud_test_node(2))).WillByDefault(Return(std::nullopt));
    leaf(1);
    EXPECT_TRUE(specializer.specialize(state));
    EXPECT_THAT(state.path, ElementsAre(pud_test_node(0), pud_test_node(1)));
    EXPECT_EQ(state.current.node, pud_test_node(1));
    EXPECT_FALSE(state.witnesses.has_value());
}

TEST_F(PudResumableCandidateSpecializerTest, ASoleWitnessingSubtreeMovesTheCandidateDownAndSearchesItsOtherChildren) {
    interior(0, {1, 2});
    ON_CALL(descend, descend(_, pud_test_node(2))).WillByDefault(Return(std::nullopt));
    interior(1, {3, 4});
    ON_CALL(resume, resume(Field(&pud_replay::node, pud_test_node(1)), _))
        .WillByDefault(Return(pud_test_node(3)));
    ON_CALL(advance, advance(_))
        .WillByDefault(Return(pud_witness_advance{.search_root = replay_at(3), .next_sibling = 1}));
    EXPECT_TRUE(specializer.specialize(state));
    EXPECT_THAT(state.path, ElementsAre(pud_test_node(0), pud_test_node(1)));
    EXPECT_EQ(state.witnesses->a->search_root.node, pud_test_node(3));
    EXPECT_EQ(state.witnesses->a->leaf, pud_test_node(3));
    EXPECT_EQ(state.witnesses->b->leaf, pud_test_node(4));
}

TEST_F(PudResumableCandidateSpecializerTest, SpecializingAgainFillsOnlyTheMissingWitnessFromTheNextChild) {
    interior(0, {1, 2, 3});
    ASSERT_TRUE(specializer.specialize(state));
    state.witnesses->b = std::nullopt;
    EXPECT_CALL(resume, resume(Field(&pud_replay::node, pud_test_node(1)), _)).Times(0);
    EXPECT_CALL(resume, resume(Field(&pud_replay::node, pud_test_node(3)), _));
    EXPECT_TRUE(specializer.specialize(state));
    EXPECT_EQ(state.witnesses->a->leaf, pud_test_node(1));
    EXPECT_EQ(state.witnesses->b->leaf, pud_test_node(3));
}
