#if 0
#include <optional>
#include <variant>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_mhcs.hpp"

using ::testing::_;
using ::testing::Return;

namespace {

struct HeadOps {
    MOCK_METHOD((std::optional<pud_candidate_resume_context<int>>), resume, ());
    MOCK_METHOD(void, witness_refuted, (pud_mhws_head_id));
};

struct MockHead {
    static HeadOps* ops;
    std::optional<pud_candidate_resume_context<int>> resume() const { return ops->resume(); }
    void witness_refuted(pud_mhws_head_id id) const { ops->witness_refuted(id); }
};

HeadOps* MockHead::ops = nullptr;

struct MockMakeHead {
    MOCK_METHOD(MockHead, make, (pud_query_position<int>));
};

struct MockForkHead {
    MOCK_METHOD(MockHead, fork, (const MockHead&, int));
};

using test_mhcs_t = pud_mhcs<int, MockHead, MockMakeHead, MockForkHead>;

struct PudMhcsTest : public ::testing::Test {
    HeadOps ops;
    MockMakeHead make_head;
    MockForkHead fork_head;
    test_mhcs_t mhcs{make_head, fork_head};
    pud_node leaf{};
    pud_node other{};

    pud_candidate_resume_context<int> self_of(const pud_node* node) {
        return pud_candidate_resume_context<int>{
            .justification = pud_candidate_self_witness{.node = node},
            .query_handle = 1};
    }

    pud_candidate_resume_context<int> choice_of(pud_mhws_head_id a, pud_mhws_head_id b) {
        return pud_candidate_resume_context<int>{
            .justification = pud_candidate_choice_point{.witness_a = a, .witness_b = b},
            .query_handle = 1};
    }

    void SetUp() override {
        MockHead::ops = &ops;
        ON_CALL(make_head, make(_)).WillByDefault(Return(MockHead{}));
        ON_CALL(fork_head, fork(_, _)).WillByDefault(Return(MockHead{}));
        ON_CALL(ops, witness_refuted(_)).WillByDefault(Return());
    }
};

TEST_F(PudMhcsTest, AddWithNoJustificationIsNotRemembered) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    EXPECT_FALSE(mhcs.try_add_head(pud_query_position<int>{.handle = 1, .node = &leaf}).has_value());
    EXPECT_TRUE(mhcs.invalidate_leaf(&leaf).empty());
}

TEST_F(PudMhcsTest, RemoveOfUnknownIdLeavesALiveCandidate) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(self_of(&leaf))).WillOnce(Return(std::nullopt));
    auto id = mhcs.try_add_head(pud_query_position<int>{.handle = 1, .node = &leaf});
    ASSERT_TRUE(id.has_value());
    mhcs.remove_head(*id + 100);
    auto gone = mhcs.invalidate_leaf(&leaf);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *id);
}

TEST_F(PudMhcsTest, InvalidateOfUnoccupiedLeafIsEmpty) {
    EXPECT_TRUE(mhcs.invalidate_leaf(&leaf).empty());
}

TEST_F(PudMhcsTest, RefuteOfUnknownWitnessIsNothing) {
    EXPECT_FALSE(mhcs.witness_refuted(4).has_value());
}

TEST_F(PudMhcsTest, ChoicePointLosesOneWitnessAndTheOtherNoLongerJustifies) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(choice_of(3, 4)))
        .WillOnce(Return(std::nullopt));
    auto id = mhcs.try_add_head(pud_query_position<int>{.handle = 1, .node = &leaf});
    ASSERT_TRUE(id.has_value());
    auto lost = mhcs.witness_refuted(3);
    ASSERT_TRUE(lost.has_value());
    EXPECT_EQ(*lost, *id);
    EXPECT_FALSE(mhcs.witness_refuted(4).has_value());
}

TEST_F(PudMhcsTest, ForkWithNoJustificationLeavesTheOriginal) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(self_of(&leaf)))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt));
    auto id = mhcs.try_add_head(pud_query_position<int>{.handle = 1, .node = &leaf});
    ASSERT_TRUE(id.has_value());
    EXPECT_FALSE(mhcs.try_fork_head(*id, 9).has_value());
    auto gone = mhcs.invalidate_leaf(&leaf);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *id);
}

TEST_F(PudMhcsTest, TwoSelfWitnessesOneFindsANewJustification) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(self_of(&leaf)))
        .WillOnce(Return(self_of(&leaf)))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(self_of(&other)))
        .WillOnce(Return(std::nullopt));
    auto first = mhcs.try_add_head(pud_query_position<int>{.handle = 1, .node = &leaf});
    auto second = mhcs.try_add_head(pud_query_position<int>{.handle = 2, .node = &leaf});
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    auto gone = mhcs.invalidate_leaf(&leaf);
    ASSERT_EQ(gone.size(), 1u);
    auto later = mhcs.invalidate_leaf(&other);
    ASSERT_EQ(later.size(), 1u);
}

TEST_F(PudMhcsTest, RemoveThenInvalidateIsEmpty) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(self_of(&leaf)));
    auto id = mhcs.try_add_head(pud_query_position<int>{.handle = 1, .node = &leaf});
    ASSERT_TRUE(id.has_value());
    mhcs.remove_head(*id);
    EXPECT_TRUE(mhcs.invalidate_leaf(&leaf).empty());
}

} // namespace
#endif
