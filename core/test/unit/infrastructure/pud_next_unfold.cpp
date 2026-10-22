#include <optional>
#include <set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_next_unfold.hpp"

using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockGetNextUnitUnfold {
    MOCK_METHOD(std::optional<pud_unfold>, get_next_unit_unfold, ());
};
struct MockGenerateDecision {
    MOCK_METHOD(pud_decision, generate, (const std::set<pud_node_id>&));
};

struct PudNextUnfoldTest : public ::testing::Test {
    StrictMock<MockGetNextUnitUnfold> unit_unfolds;
    StrictMock<MockGenerateDecision> decisions;
    pud_next_unfold<StrictMock<MockGetNextUnitUnfold>, StrictMock<MockGenerateDecision>> next{unit_unfolds, decisions};
    std::set<pud_node_id> open{4, 9};
};

}  // namespace

TEST_F(PudNextUnfoldTest, ForcedUnitUnfoldDominatesAndNoDecisionIsMade) {
    EXPECT_CALL(unit_unfolds, get_next_unit_unfold()).WillOnce(Return(pud_unfold{9, 2}));
    EXPECT_EQ(next.get_next_unfold(open), (pud_unfold{9, 2}));
}

TEST_F(PudNextUnfoldTest, ForcedUnitUnfoldNeedNotBeAmongTheOpenLeaves) {
    EXPECT_CALL(unit_unfolds, get_next_unit_unfold()).WillOnce(Return(pud_unfold{30, 0}));
    EXPECT_EQ(next.get_next_unfold(open), (pud_unfold{30, 0}));
}

TEST_F(PudNextUnfoldTest, DecisionAmongTheOpenLeavesIsMadeOnlyWhenNothingIsForced) {
    InSequence in_order;
    EXPECT_CALL(unit_unfolds, get_next_unit_unfold()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(decisions, generate(open)).WillOnce(Return(pud_decision{4, 1}));
    EXPECT_EQ(next.get_next_unfold(open), (pud_unfold{4, 1}));
}
