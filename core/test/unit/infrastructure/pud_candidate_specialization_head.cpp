#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_candidate_specialization_head.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

using child_iter = std::vector<pud_node_id>::const_iterator;

struct descent_t {
    pud_node_id node;
    bool operator==(const descent_t&) const = default;
};

using advance_result_t = pud_witness_advance_result<descent_t, child_iter>;

struct MockTryAddHead {
    MOCK_METHOD(std::optional<pud_mhws_head_id>, try_add_head, (descent_t));
};
struct MockAdvanceHead {
    MOCK_METHOD((std::optional<advance_result_t>), advance_head, (pud_mhws_head_id));
};
struct MockForkWitnessHead {
    MOCK_METHOD(std::optional<pud_mhws_head_id>, try_fork_head, (pud_mhws_head_id, descent_t));
};
struct MockCheckLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_node_id));
};
struct MockGetChildren {
    MOCK_METHOD((const std::vector<pud_node_id>&), get, (pud_node_id));
};
struct MockDescend {
    MOCK_METHOD(std::optional<descent_t>, descend, (descent_t, pud_node_id));
};

using test_head_t = pud_candidate_specialization_head<
    descent_t, child_iter,
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
            descent_t{node}};
    }

    void SetUp() override {
        ON_CALL(leaves, check_leaf(_)).WillByDefault(Return(false));
        ON_CALL(children, get(_)).WillByDefault(
            [&](pud_node_id id) -> const std::vector<pud_node_id>& {
                return sequences.at(id);
            });
        ON_CALL(descend, descend(_, _)).WillByDefault(
            [](descent_t, pud_node_id child) {
                return std::optional<descent_t>{descent_t{child}};
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
    EXPECT_EQ(found->descent, descent_t{root});
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
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(descent_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(cp->witness_a, 10u);
    EXPECT_EQ(cp->witness_b, 11u);
    EXPECT_EQ(found->descent, descent_t{root});
}

TEST_F(PudCandidateSpecializationHeadTest, FirstChildBlockedNextTwoFormChoicePoint) {
    sequences[root] = {a, b, c};
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(descent_t{b})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(descent_t{c})).WillOnce(Return(pud_mhws_head_id{11}));
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
    ON_CALL(descend, descend(descent_t{root}, a)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(descent_t{b})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(descent_t{c})).WillOnce(Return(pud_mhws_head_id{11}));
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
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(descent_t{b})).WillOnce(Return(pud_mhws_head_id{10}));
    ON_CALL(leaves, check_leaf(b)).WillByDefault(Return(true));
    advance_result_t ar{
        .root_descent          = descent_t{b},
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
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(pud_mhws_head_id{4}));
    advance_result_t ar{
        .root_descent          = descent_t{a},
        .root_next_sibling_it = sequences[a].begin(),
        .root_end_sibling_it  = sequences[a].end()};
    EXPECT_CALL(advance_head, advance_head(4u)).WillOnce(Return(ar));
    EXPECT_CALL(try_add, try_add_head(descent_t{a1})).WillOnce(Return(pud_mhws_head_id{20}));
    auto head = make_head(root);
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(cp->witness_a, 4u);
    EXPECT_EQ(cp->witness_b, 20u);
    EXPECT_EQ(found->descent, descent_t{a});
}

// advance_head returns nullopt (witness exhausted): head resets and re-searches
// from children of the new position  (B4: crash — deref before null check)
TEST_F(PudCandidateSpecializationHeadTest, SingleWitnessExhaustedResetsAndFindsChoicePoint) {
    sequences[root] = {a};
    sequences[a] = {a1, a2};
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(pud_mhws_head_id{4}));
    EXPECT_CALL(advance_head, advance_head(4u)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(descent_t{a1})).WillOnce(Return(pud_mhws_head_id{20}));
    EXPECT_CALL(try_add, try_add_head(descent_t{a2})).WillOnce(Return(pud_mhws_head_id{21}));
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
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(descent_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    // witness 10 (child a) refuted; child c should be the replacement
    EXPECT_CALL(try_add, try_add_head(descent_t{c})).WillOnce(Return(pud_mhws_head_id{12}));
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
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(descent_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    ON_CALL(leaves, check_leaf(b)).WillByDefault(Return(true));
    advance_result_t ar{
        .root_descent          = descent_t{b},
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
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(descent_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
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
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(descent_t{b})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(advance_head, advance_head(10u)).WillOnce(Return(std::nullopt));
    ON_CALL(leaves, check_leaf(b)).WillByDefault(Return(true));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    // fork: new handle 50 cannot descend into b → path truncated → nullopt
    EXPECT_CALL(descend, descend(descent_t{50}, b)).WillOnce(Return(std::nullopt));
    test_head_t forked{head, descent_t{50}};
    EXPECT_FALSE(forked.resume().has_value());
}

// Fork of a self-witness: no path to replay, current_descent_ must be the new root
// (B10: copy ctor never sets current_descent_, so it stays descent_t{0})
TEST_F(PudCandidateSpecializationHeadTest, ForkOfSelfWitnessHasCorrectDescent) {
    ON_CALL(leaves, check_leaf(root)).WillByDefault(Return(true));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    // node_path_ is empty for a self-witness; copy ctor loop has zero iterations;
    // current_descent_ must be set to the fork root, not default-constructed descent_t{}
    ON_CALL(leaves, check_leaf(pud_node_id{99})).WillByDefault(Return(true));
    test_head_t forked{head, descent_t{99}};
    auto found = forked.resume();
    ASSERT_TRUE(found.has_value());
    // The forked head's descent must be the new root (not default-constructed descent_t{})
    EXPECT_EQ(found->descent, descent_t{99});
    const auto* self = std::get_if<pud_candidate_self_witness>(&found->justification);
    ASSERT_NE(self, nullptr);
    // forked head is at node 99 (current_descent_.node), so self-witness is at 99
    EXPECT_EQ(self->node, pud_node_id{99});
}

// Fork of a choice-point: both witnesses must be forked with the new root
TEST_F(PudCandidateSpecializationHeadTest, ForkOfChoicePointForksWitnesses) {
    sequences[root] = {a, b};
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(descent_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    // Fork: new handle 99, descend into a and b gives handles 22 and 33
    EXPECT_CALL(descend, descend(descent_t{99}, a)).WillOnce(Return(descent_t{22}));
    EXPECT_CALL(descend, descend(descent_t{99}, b)).WillOnce(Return(descent_t{33}));
    EXPECT_CALL(fork_witness, try_fork_head(10u, descent_t{22})).WillOnce(Return(pud_mhws_head_id{100}));
    EXPECT_CALL(fork_witness, try_fork_head(11u, descent_t{33})).WillOnce(Return(pud_mhws_head_id{101}));
    test_head_t forked{head, descent_t{99}};
    auto found = forked.resume();
    ASSERT_TRUE(found.has_value());
    const auto* cp = std::get_if<pud_candidate_choice_point>(&found->justification);
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(cp->witness_a, 100u);
    EXPECT_EQ(cp->witness_b, 101u);
}

// ── witness_b refuted (not witness_a) — the else branch ──────────────────────

// witness_refuted() has two branches: the if checks witness_a, the else
// clears witness_b. The existing "NoReplacement" test only exercises the
// if branch (refutes a). This test refutes b so the else branch is reached.
TEST_F(PudCandidateSpecializationHeadTest, WitnessBRefutedAAdvancesToSelfWitness) {
    sequences[root] = {a, b};
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(descent_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    // refute b (id 11); a (id 10) is the sole surviving witness
    ON_CALL(leaves, check_leaf(a)).WillByDefault(Return(true));
    advance_result_t ar{
        .root_descent         = descent_t{a},
        .root_next_sibling_it = sequences[root].end(),
        .root_end_sibling_it  = sequences[root].end()};
    EXPECT_CALL(advance_head, advance_head(10u)).WillOnce(Return(ar));
    head.witness_refuted(11);
    auto next = head.resume();
    ASSERT_TRUE(next.has_value());
    const auto* self = std::get_if<pud_candidate_self_witness>(&next->justification);
    ASSERT_NE(self, nullptr);
    EXPECT_EQ(self->node, a);
}

// ── witness_refuted for a witness this head does not own ─────────────────────

TEST_F(PudCandidateSpecializationHeadTest, WitnessRefutedForUnownedWitnessAsserts) {
    sequences[root] = {a, b};
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(descent_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    // head owns witnesses 10 and 11 only
    EXPECT_THROW(head.witness_refuted(99), std::logic_error);
}

// A self-witness head has no witness scan at all, so it owns no witnesses.
TEST_F(PudCandidateSpecializationHeadTest, WitnessRefutedOnSelfWitnessHeadAsserts) {
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(true));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    EXPECT_THROW(head.witness_refuted(10), std::logic_error);
}

// witness_a was already refuted; refuting it again must not be silently accepted.
TEST_F(PudCandidateSpecializationHeadTest, WitnessRefutedTwiceAsserts) {
    sequences[root] = {a, b};
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(descent_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    head.witness_refuted(10);
    EXPECT_THROW(head.witness_refuted(10), std::logic_error);
}

// ── Fork where one witness cannot be re-forked ────────────────────────────────

// try_fork_head() returns nullopt for witness_a; witness_b succeeds. The forked
// head must advance toward the sole surviving witness (b) and produce a result.
TEST_F(PudCandidateSpecializationHeadTest, ForkWhereWitnessACannotBeForkededAdvancesToB) {
    sequences[root] = {a, b};
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(descent_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    ASSERT_TRUE(head.resume().has_value());
    EXPECT_CALL(descend, descend(descent_t{99}, a)).WillOnce(Return(descent_t{22}));
    EXPECT_CALL(descend, descend(descent_t{99}, b)).WillOnce(Return(descent_t{33}));
    EXPECT_CALL(fork_witness, try_fork_head(10u, descent_t{22})).WillOnce(Return(std::nullopt));
    EXPECT_CALL(fork_witness, try_fork_head(11u, descent_t{33})).WillOnce(Return(pud_mhws_head_id{101}));
    test_head_t forked{head, descent_t{99}};
    // forked has witness_a=nullopt, witness_b={101, descent_t{33}}; advance exhausts it
    EXPECT_CALL(advance_head, advance_head(101u)).WillOnce(Return(std::nullopt));
    ON_CALL(leaves, check_leaf(pud_node_id{33})).WillByDefault(Return(true));
    auto found = forked.resume();
    ASSERT_TRUE(found.has_value());
    const auto* self = std::get_if<pud_candidate_self_witness>(&found->justification);
    ASSERT_NE(self, nullptr);
    EXPECT_EQ(self->node, pud_node_id{33});
}

// ── Resume idempotence ────────────────────────────────────────────────────────

// resume() must not consume its own result: calling it twice in a row without
// any intervening advance()/witness_refuted() must yield the same context.

TEST_F(PudCandidateSpecializationHeadTest, SelfWitnessResumeIsIdempotent) {
    EXPECT_CALL(leaves, check_leaf(root)).WillRepeatedly(Return(true));
    auto head = make_head(root);
    auto first  = head.resume();
    auto second = head.resume();
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(std::get<pud_candidate_self_witness>(first->justification).node,
              std::get<pud_candidate_self_witness>(second->justification).node);
    EXPECT_EQ(first->descent, second->descent);
}

TEST_F(PudCandidateSpecializationHeadTest, ChoicePointResumeIsIdempotent) {
    sequences[root] = {a, b};
    EXPECT_CALL(try_add, try_add_head(descent_t{a})).WillOnce(Return(pud_mhws_head_id{10}));
    EXPECT_CALL(try_add, try_add_head(descent_t{b})).WillOnce(Return(pud_mhws_head_id{11}));
    auto head = make_head(root);
    auto first  = head.resume();
    auto second = head.resume();  // must NOT call try_add_head again
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    const auto* cp1 = std::get_if<pud_candidate_choice_point>(&first->justification);
    const auto* cp2 = std::get_if<pud_candidate_choice_point>(&second->justification);
    ASSERT_NE(cp1, nullptr);
    ASSERT_NE(cp2, nullptr);
    EXPECT_EQ(cp1->witness_a, cp2->witness_a);
    EXPECT_EQ(cp1->witness_b, cp2->witness_b);
}
