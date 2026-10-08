#include <optional>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_mhcs.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct HeadOps {
    MOCK_METHOD((std::optional<pud_candidate_resume_context<int>>), resume, ());
    MOCK_METHOD(void, witness_refuted, (pud_mhws_head_id));
};

struct MockHead {
    static HeadOps* ops;
    std::optional<pud_candidate_resume_context<int>> resume() { return ops->resume(); }
    void witness_refuted(pud_mhws_head_id id) { ops->witness_refuted(id); }
};

HeadOps* MockHead::ops = nullptr;

struct MockMakeHead {
    MOCK_METHOD(MockHead, make, (int));
};
struct MockForkHead {
    MOCK_METHOD(MockHead, fork, (const MockHead&, int));
};

using test_mhcs_t = pud_mhcs<int, MockHead, MockMakeHead, MockForkHead>;

struct PudMhcsTest : public ::testing::Test {
    HeadOps           ops;
    NiceMock<MockMakeHead> make_head;
    NiceMock<MockForkHead> fork_head;
    test_mhcs_t       mhcs{make_head, fork_head};
    pud_node_id       leaf  = 1;
    pud_node_id       other = 2;

    pud_candidate_resume_context<int> self_of(pud_node_id node) {
        return {.justification = pud_candidate_self_witness{.node = node},
                .query_handle  = 7};
    }

    pud_candidate_resume_context<int> choice_of(pud_mhws_head_id wa, pud_mhws_head_id wb) {
        return {.justification = pud_candidate_choice_point{.witness_a = wa, .witness_b = wb},
                .query_handle  = 7};
    }

    void SetUp() override {
        MockHead::ops = &ops;
        ON_CALL(make_head, make(_)).WillByDefault(Return(MockHead{}));
        ON_CALL(fork_head, fork(_, _)).WillByDefault(Return(MockHead{}));
        ON_CALL(ops, witness_refuted(_)).WillByDefault(Return());
    }
};

// ── try_add_head ─────────────────────────────────────────────────────────────

TEST_F(PudMhcsTest, AddWithNoJustificationIsNotRemembered) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    EXPECT_FALSE(mhcs.try_add_head(1).has_value());
}

TEST_F(PudMhcsTest, AddSelfWitnessIsRemembered) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(self_of(leaf)));
    auto id = mhcs.try_add_head(1);
    ASSERT_TRUE(id.has_value());
}

TEST_F(PudMhcsTest, AddChoicePointIsRemembered) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(choice_of(3, 4)));
    auto id = mhcs.try_add_head(1);
    ASSERT_TRUE(id.has_value());
}

// ── remove_head ──────────────────────────────────────────────────────────────

TEST_F(PudMhcsTest, RemoveOfUnknownIdIsNoop) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(self_of(leaf)));
    auto id = mhcs.try_add_head(1);
    ASSERT_TRUE(id.has_value());
    mhcs.remove_head(*id + 100);  // no crash
    // original head still valid
    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    auto gone = mhcs.invalidate_leaf(leaf);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *id);
}

TEST_F(PudMhcsTest, RemoveThenInvalidateIsEmpty) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(self_of(leaf)));
    auto id = mhcs.try_add_head(1);
    ASSERT_TRUE(id.has_value());
    mhcs.remove_head(*id);
    EXPECT_TRUE(mhcs.invalidate_leaf(leaf).empty());
}

TEST_F(PudMhcsTest, RemoveChoicePointUnlinksWitnesses) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(choice_of(3, 4)));
    auto id = mhcs.try_add_head(1);
    ASSERT_TRUE(id.has_value());
    mhcs.remove_head(*id);
    // witnesses 3 and 4 must no longer map to anything
    EXPECT_FALSE(mhcs.witness_refuted(3).has_value());
    EXPECT_FALSE(mhcs.witness_refuted(4).has_value());
}

// ── invalidate_leaf ───────────────────────────────────────────────────────────

// Exposes B10: unlink_self_witnesses calls extracted.mapped() without empty guard
TEST_F(PudMhcsTest, InvalidateOfUnoccupiedLeafIsEmpty) {
    EXPECT_TRUE(mhcs.invalidate_leaf(leaf).empty());
}

TEST_F(PudMhcsTest, SelfWitnessInvalidatedHeadGone) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(self_of(leaf)))
        .WillOnce(Return(std::nullopt));
    auto id = mhcs.try_add_head(1);
    ASSERT_TRUE(id.has_value());
    auto gone = mhcs.invalidate_leaf(leaf);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *id);
}

TEST_F(PudMhcsTest, SelfWitnessInvalidatedHeadFindsNewSelfWitness) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(self_of(leaf)))
        .WillOnce(Return(self_of(other)));
    auto id = mhcs.try_add_head(1);
    ASSERT_TRUE(id.has_value());
    auto gone = mhcs.invalidate_leaf(leaf);
    EXPECT_TRUE(gone.empty());
    // now registered under `other`; invalidating `other` should report the head gone
    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    auto later = mhcs.invalidate_leaf(other);
    ASSERT_EQ(later.size(), 1u);
    EXPECT_EQ(later[0], *id);
}

TEST_F(PudMhcsTest, SelfWitnessInvalidatedHeadFindsChoicePoint) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(self_of(leaf)))
        .WillOnce(Return(choice_of(10, 11)));
    auto id = mhcs.try_add_head(1);
    ASSERT_TRUE(id.has_value());
    auto gone = mhcs.invalidate_leaf(leaf);
    EXPECT_TRUE(gone.empty());
    // now a choice-point: its witnesses must be registered
    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    auto result = mhcs.witness_refuted(10);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, *id);
}

TEST_F(PudMhcsTest, TwoSelfWitnessesOnSameLeafBothNotified) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(self_of(leaf)))
        .WillOnce(Return(self_of(leaf)))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt));
    auto first  = mhcs.try_add_head(1);
    auto second = mhcs.try_add_head(2);
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    auto gone = mhcs.invalidate_leaf(leaf);
    EXPECT_EQ(gone.size(), 2u);
}

// ── witness_refuted ───────────────────────────────────────────────────────────

TEST_F(PudMhcsTest, RefuteOfUnknownWitnessReturnsNullopt) {
    EXPECT_FALSE(mhcs.witness_refuted(42).has_value());
}

TEST_F(PudMhcsTest, ChoicePointWitnessRefutedHeadNotified) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(choice_of(3, 4)))
        .WillOnce(Return(choice_of(5, 4)));
    auto id = mhcs.try_add_head(1);
    ASSERT_TRUE(id.has_value());
    EXPECT_CALL(ops, witness_refuted(3u));
    auto result = mhcs.witness_refuted(3);
    EXPECT_FALSE(result.has_value());  // head survived with new context
    // new witnesses 5 and 4 must be registered
    EXPECT_CALL(ops, witness_refuted(5u));
    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    auto lost = mhcs.witness_refuted(5);
    ASSERT_TRUE(lost.has_value());
    EXPECT_EQ(*lost, *id);
}

TEST_F(PudMhcsTest, ChoicePointBothWitnessesRefutedHeadGone) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(choice_of(3, 4)))
        .WillOnce(Return(std::nullopt));
    auto id = mhcs.try_add_head(1);
    ASSERT_TRUE(id.has_value());
    auto lost = mhcs.witness_refuted(3);
    ASSERT_TRUE(lost.has_value());
    EXPECT_EQ(*lost, *id);
    // witness 4 was unlinked when head was destroyed; refuting it is a noop
    EXPECT_FALSE(mhcs.witness_refuted(4).has_value());
}

// After witness_refuted triggers a resume that re-uses the surviving witness,
// that witness must remain accessible for future refutation
TEST_F(PudMhcsTest, SurvivingWitnessReregisteredAfterRefute) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(choice_of(3, 4)))
        .WillOnce(Return(choice_of(5, 4)));  // 4 survives, 5 is new
    auto id = mhcs.try_add_head(1);
    ASSERT_TRUE(id.has_value());
    mhcs.witness_refuted(3);
    // witness 4 should still be registered and route to the same head
    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    auto lost = mhcs.witness_refuted(4);
    ASSERT_TRUE(lost.has_value());
    EXPECT_EQ(*lost, *id);
}

// ── try_fork_head ─────────────────────────────────────────────────────────────

TEST_F(PudMhcsTest, ForkWithNoJustificationDoesNotAddHead) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(self_of(leaf)))
        .WillOnce(Return(std::nullopt));
    auto id = mhcs.try_add_head(1);
    ASSERT_TRUE(id.has_value());
    EXPECT_FALSE(mhcs.try_fork_head(*id, 99).has_value());
}

TEST_F(PudMhcsTest, ForkWithJustificationAddsNewHead) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(self_of(leaf)))
        .WillOnce(Return(self_of(other)));
    auto id = mhcs.try_add_head(1);
    ASSERT_TRUE(id.has_value());
    auto forked = mhcs.try_fork_head(*id, 99);
    ASSERT_TRUE(forked.has_value());
    EXPECT_NE(*forked, *id);
    // forked head is registered under `other`
    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    auto gone = mhcs.invalidate_leaf(other);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *forked);
}

} // namespace
