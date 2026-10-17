#include <set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_next_decided_unfold.hpp"

using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockGenerateDecision {
    MOCK_METHOD(pud_decision, generate, (const std::set<pud_node_id>&));
};

}  // namespace

TEST(PudNextDecidedUnfoldTest, EveryUnfoldIsTheDecisionOverTheOpenLeaves) {
    StrictMock<MockGenerateDecision> decisions;
    pud_next_decided_unfold<StrictMock<MockGenerateDecision>> next{decisions};
    const std::set<pud_node_id> open{4, 9};
    EXPECT_CALL(decisions, generate(open)).WillOnce(Return(pud_decision{9, 2}));
    EXPECT_EQ(next.get_next_unfold(open), (pud_unfold{9, 2}));
}
