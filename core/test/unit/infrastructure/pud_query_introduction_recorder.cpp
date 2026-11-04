#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_query_introduction_recorder.hpp"
#include "pud_test_node.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;

namespace {

struct MockIntroduceQuery {
    MOCK_METHOD(void, introduce, (pud_mhq_head_id, pud_global_node_id, pud_body_goal_id));
};
struct MockWithdrawIntroduction {
    MOCK_METHOD(void, withdraw, (pud_mhq_head_id));
};

struct PudQueryIntroductionRecorderTest : public ::testing::Test {
    NiceMock<MockIntroduceQuery> introducers;
    NiceMock<MockWithdrawIntroduction> withdrawals;
    pud_query_introduction_recorder<NiceMock<MockIntroduceQuery>, NiceMock<MockWithdrawIntroduction>> recorder{
        introducers, withdrawals};
};

}

TEST_F(PudQueryIntroductionRecorderTest, AnIntroductionIsForwardedAndLogged) {
    EXPECT_CALL(introducers, introduce(40u, pud_test_node(3), 2u));
    recorder.introduce(40, pud_test_node(3), 2);
    EXPECT_THAT(
        recorder.take_introduced(),
        ElementsAre(pud_introduced_query{.head = 40, .node = pud_test_node(3), .goal = 2}));
}

TEST_F(PudQueryIntroductionRecorderTest, AWithdrawalIsForwardedAndLogged) {
    EXPECT_CALL(withdrawals, withdraw(40u));
    recorder.withdraw(40);
    EXPECT_THAT(recorder.take_withdrawn(), ElementsAre(40u));
}

TEST_F(PudQueryIntroductionRecorderTest, TakingALogEmptiesItAndLeavesTheOtherAlone) {
    recorder.introduce(40, pud_test_node(3), 2);
    recorder.withdraw(41);
    EXPECT_EQ(recorder.take_introduced().size(), 1u);
    EXPECT_THAT(recorder.take_introduced(), IsEmpty());
    EXPECT_THAT(recorder.take_withdrawn(), ElementsAre(41u));
    EXPECT_THAT(recorder.take_withdrawn(), IsEmpty());
}
