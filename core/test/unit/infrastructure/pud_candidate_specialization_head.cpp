#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_candidate_specialization_head.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

using child_iter = std::vector<pud_node_id>::const_iterator;

struct handle_t {
    pud_node_id node;
    bool operator==(const handle_t&) const = default;
};

using advance_result_t = pud_witness_advance_result<handle_t, child_iter>;

struct MockTryAddHead {
    MOCK_METHOD(std::optional<pud_mhws_head_id>, try_add_head, (handle_t));
};
struct MockAdvanceHead {
    MOCK_METHOD((std::optional<advance_result_t>), advance_head, (pud_mhws_head_id));
};
struct MockForkWitnessHead {
    MOCK_METHOD(std::optional<pud_mhws_head_id>, try_fork_head, (pud_mhws_head_id, handle_t));
};
struct MockCheckLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_node_id));
};
struct MockGetChildren {
    MOCK_METHOD((const std::vector<pud_node_id>&), get, (pud_node_id));
};
struct MockDescend {
    MOCK_METHOD(std::optional<handle_t>, descend, (handle_t, pud_node_id));
};

using test_head_t = pud_candidate_specialization_head<
    handle_t, child_iter,
    MockTryAddHead, MockAdvanceHead, MockForkWitnessHead,
    MockCheckLeaf, MockGetChildren, MockDescend>;

struct PudCandidateSpecializationHeadTest : public ::testing::Test {
    NiceMock<MockTryAddHead>    try_add;
    NiceMock<MockAdvanceHead>   advance_head;
    NiceMock<MockForkWitnessHead> fork_witness;
    NiceMock<MockCheckLeaf>     leaves;
    NiceMock<MockGetChildren>   children;
    NiceMock<MockDescend>       descend;
    pud_node_id root = 1;
    pud_node_id a    = 2;
    pud_node_id b    = 3;
    pud_node_id c    = 4;
    pud_node_id a1   = 5;
    pud_node_id a2   = 6;
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> sequences;

    test_head_t make_head(pud_node_id node) {
        return test_head_t{
            try_add, advance_head, fork_witness, leaves, children, descend,
            handle_t{node}};
    }

    void SetUp() override {
        ON_CALL(leaves, check_leaf(_)).WillByDefault(Return(false));
        ON_CALL(children, get(_)).WillByDefault(
            [&](pud_node_id id) -> const std::vector<pud_node_id>& {
                return sequences.at(id);
            });
        ON_CALL(descend, descend(_, _)).WillByDefault(
            [](handle_t, pud_node_id child) {
                return std::optional<handle_t>{handle_t{child}};
            });
    }
};

// ── Self-witness: root is a leaf ──────────────────────────────────────────────

TEST_F(PudCandidateSpecializationHeadTest, LeafRootIsASelfWitness) {
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(true));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* self = std::get_if<pud_candidate_self_witness>(&found->justification);
    ASSERT_NE(self, nullptr);
    EXPECT_EQ(self->node, root);
    EXPECT_EQ(found->query_handle, handle_t{root});
}

// ── No justification ─────────────────────────────────────────────────────────

TEST_F(PudCandidateSpecializationHeadTest, NoChildrenYieldsNoJustification) {
    sequences[root] = {};
    auto head = make_head(root);
    EXPECT_FALSE(head.resume().has_value());
}

TEST_F(PudCandidateSpecializationHeadTest, AllChildrenBlockedYieldsNoJustification) {
    sequences[root] = {a, b};
    EXPECT_CALL(try_add, try_add_head(_)).WillRepeatedly(Return(std::nullopt));
    auto head = make_head(root);
    EXPECT_FALSE(head.resume().has_value());
}

// ── Choice point ─────────────────────────────────────────────────────────────

// Each witness must come from a DIFFERENT child; verifies iterator is advanced
// after the first witness is found (B6: iterator not advanced → both from child a)
TEST_F(PudCandidateSpecializationHeadTest, TwoWitnessesFromDistinctChildrenFormChoicePoint) {
    sequences[root] = {a, b};
    EXPECT_CALL(try_add, try_add_head(handle_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(handle_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(cp->witness_a, 10u);
    EXPECT_EQ(cp->witness_b, 11u);
    EXPECT_EQ(found->query_handle, handle_t{root});
}

TEST_F(PudCandidateSpecializationHeadTest, FirstChildBlockedNextTwoFormChoicePoint) {
    sequences[root] = {a, b, c};
    EXPECT_CALL(try_add, try_add_head(handle_t{a})).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(handle_t{b})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(handle_t{c})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(cp->witness_a, 10u);
    EXPECT_EQ(cp->witness_b, 11u);
}

// A child not reachable by the query (descend returns nullopt) must be skipped;
// verifies no .value() crash (B7)
TEST_F(PudCandidateSpecializationHeadTest, UnreachableChildSkippedAndNextTwoFormChoicePoint) {
    sequences[root] = {a, b, c};
    ON_CALL(descend, descend(handle_t{root}, a)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(handle_t{b})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(handle_t{c})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(cp->witness_a, 10u);
    EXPECT_EQ(cp->witness_b, 11u);
}

// ── Single witness: advance ───────────────────────────────────────────────────

// One surviving witness; advance_head returns a result → self-witness at that leaf
TEST_F(PudCandidateSpecializationHeadTest, SingleWitnessAdvancesToSelfWitness) {
    sequences[root] = {a, b};
    // a has no leaf, b does
    EXPECT_CALL(try_add, try_add_head(handle_t{a})).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(handle_t{b})).WillOnce(Return(pud_mhws_head_id{10}));
    ON_CALL(leaves, check_leaf(b)).WillByDefault(Return(true));
    advance_result_t ar{
        .root_handle          = handle_t{b},
        .root_next_sibling_it = sequences[root].end(),
        .root_end_sibling_it  = sequences[root].end()};
    EXPECT_CALL(advance_head, advance_head(10u)).WillOnce(Return(ar));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* self = std::get_if<pud_candidate_self_witness>(&found->justification);
    ASSERT_NE(self, nullptr);
    EXPECT_EQ(self->node, b);
}

// One surviving witness; advance_head result exposes siblings → choice point
TEST_F(PudCandidateSpecializationHeadTest, SingleWitnessAdvancesToChoicePoint) {
    sequences[root] = {a};
    sequences[a] = {a1, a2};
    EXPECT_CALL(try_add, try_add_head(handle_t{a})).WillOnce(Return(pud_mhws_head_id{4}));
    advance_result_t ar{
        .root_handle          = handle_t{a},
        .root_next_sibling_it = sequences[a].begin(),
        .root_end_sibling_it  = sequences[a].end()};
    EXPECT_CALL(advance_head, advance_head(4u)).WillOnce(Return(ar));
    EXPECT_CALL(try_add, try_add_head(handle_t{a1})).WillOnce(Return(pud_mhws_head_id{20}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(cp->witness_a, 4u);
    EXPECT_EQ(cp->witness_b, 20u);
    EXPECT_EQ(found->query_handle, handle_t{a});
}

// advance_head returns nullopt (witness exhausted): head resets and re-searches
// from children of the new position  (B4: crash — deref before null check)
TEST_F(PudCandidateSpecializationHeadTest, SingleWitnessExhaustedResetsAndFindsChoicePoint) {
    sequences[root] = {a};
    sequences[a] = {a1, a2};
    EXPECT_CALL(try_add, try_add_head(handle_t{a})).WillOnce(Return(pud_mhws_head_id{4}));
    EXPECT_CALL(advance_head, advance_head(4u)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(handle_t{a1})).WillOnce(Return(pud_mhws_head_id{20}));
    EXPECT_CALL(try_add, try_add_head(handle_t{a2})).WillOnce(Return(pud_mhws_head_id{21}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(cp->witness_a, 20u);
    EXPECT_EQ(cp->witness_b, 21u);
}

// ── witness_refuted ───────────────────────────────────────────────────────────

TEST_F(PudCandidateSpecializationHeadTest, WitnessRefutedReplacedFromNextChild) {
    sequences[root] = {a, b, c};
    EXPECT_CALL(try_add, try_add_head(handle_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(handle_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    // witness 10 (child a) refuted; child c should be the replacement
    EXPECT_CALL(try_add, try_add_head(handle_t{c})).WillOnce(Return(pud_mhws_head_id{12}));
    head.witness_refuted(10);
    auto next = head.resume();
    ASSERT_TRUE(next.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&next->justification);
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(cp->witness_a, 12u);
    EXPECT_EQ(cp->witness_b, 11u);
}

TEST_F(PudCandidateSpecializationHeadTest, WitnessRefutedNoReplacementAdvancesToSurvivor) {
    sequences[root] = {a, b};
    EXPECT_CALL(try_add, try_add_head(handle_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(handle_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    ON_CALL(leaves, check_leaf(b)).WillByDefault(Return(true));
    advance_result_t ar{
        .root_handle          = handle_t{b},
        .root_next_sibling_it = sequences[root].end(),
        .root_end_sibling_it  = sequences[root].end()};
    EXPECT_CALL(advance_head, advance_head(11u)).WillOnce(Return(ar));
    head.witness_refuted(10);  // a gone; b is the survivor
    auto next = head.resume();
    ASSERT_TRUE(next.has_value());
    const auto* self = std::get_if<pud_candidate_self_witness>(&next->justification);
    ASSERT_NE(self, nullptr);
    EXPECT_EQ(self->node, b);
}

TEST_F(PudCandidateSpecializationHeadTest, BothWitnessesRefutedYieldsNullopt) {
    sequences[root] = {a, b};
    EXPECT_CALL(try_add, try_add_head(handle_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(handle_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    head.witness_refuted(10);
    head.witness_refuted(11);
    EXPECT_FALSE(head.resume().has_value());
}

// ── Copy ctor (fork) ──────────────────────────────────────────────────────────

// New query handle cannot descend into the path → truncated → no justification
TEST_F(PudCandidateSpecializationHeadTest, ForkCannotFollowPathYieldsNullopt) {
    // build a head that advanced to a self-witness at b; node_path_ = [b]
    sequences[root] = {a, b};
    EXPECT_CALL(try_add, try_add_head(handle_t{a})).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(handle_t{b})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(advance_head, advance_head(10u)).WillOnce(Return(std::nullopt));
    ON_CALL(leaves, check_leaf(b)).WillByDefault(Return(true));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    // fork: new handle 50 cannot descend into b → path truncated → nullopt
    EXPECT_CALL(descend, descend(handle_t{50}, b)).WillOnce(Return(std::nullopt));
    test_head_t forked{head, handle_t{50}};
    EXPECT_FALSE(forked.resume().has_value());
}

// Fork of a self-witness: no path to replay, current_handle_ must be the new root
// (B10: copy ctor never sets current_handle_, so it stays handle_t{0})
TEST_F(PudCandidateSpecializationHeadTest, ForkOfSelfWitnessHasCorrectHandle) {
    ON_CALL(leaves, check_leaf(root)).WillByDefault(Return(true));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    // node_path_ is empty for a self-witness; copy ctor loop has zero iterations;
    // current_handle_ must be set to the fork root, not default-constructed handle_t{}
    ON_CALL(leaves, check_leaf(pud_node_id{99})).WillByDefault(Return(true));
    test_head_t forked{head, handle_t{99}};
    auto found = forked.resume();
    ASSERT_TRUE(found.has_value());
    // The forked head's query_handle must be the new root handle (99), not 0
    EXPECT_EQ(found->query_handle, handle_t{99});
    const auto* self = std::get_if<pud_candidate_self_witness>(&found->justification);
    ASSERT_NE(self, nullptr);
    // forked head is at node 99 (current_handle_.node), so self-witness is at 99
    EXPECT_EQ(self->node, pud_node_id{99});
}

// Fork of a choice-point: both witnesses must be forked with the new root
TEST_F(PudCandidateSpecializationHeadTest, ForkOfChoicePointForksWitnesses) {
    sequences[root] = {a, b};
    EXPECT_CALL(try_add, try_add_head(handle_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(handle_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    // Fork: new handle 99, descend into a and b gives handles 22 and 33
    EXPECT_CALL(descend, descend(handle_t{99}, a)).WillOnce(Return(handle_t{22}));
    EXPECT_CALL(descend, descend(handle_t{99}, b)).WillOnce(Return(handle_t{33}));
    EXPECT_CALL(fork_witness, try_fork_head(10u, handle_t{22})).WillOnce(Return(pud_mhws_head_id{100}));
    EXPECT_CALL(fork_witness, try_fork_head(11u, handle_t{33})).WillOnce(Return(pud_mhws_head_id{101}));
    test_head_t forked{head, handle_t{99}};
    auto found = forked.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(cp->witness_a, 100u);
    EXPECT_EQ(cp->witness_b, 101u);
}
