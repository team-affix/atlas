// dbuct_joint_elimination_generator: CDCL yields drop MHU heads then stream; MHU yields forward only.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/dbuct_joint_elimination_generator.hpp"
#include "infrastructure/coroutine.hpp"

using ::testing::ByMove;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

coroutine<const resolution_lineage*, void> make_single_elim_sm(
    const resolution_lineage* elim) {
    co_yield elim;
}

coroutine<const resolution_lineage*, void> empty_elim_sm() {
    co_return;
}

std::vector<const resolution_lineage*> collect_elims(
    coroutine<const resolution_lineage*, void>& sm) {
    std::vector<const resolution_lineage*> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            out.push_back(sm.consume_yield());
    }
    return out;
}

struct MockConstrainCdcl {
    MOCK_METHOD((coroutine<const resolution_lineage*, void>), constrain,
        (const resolution_lineage*));
};

struct MockConstrainMhu {
    MOCK_METHOD((coroutine<const resolution_lineage*, void>), constrain,
        (const resolution_lineage*));
    MOCK_METHOD(void, remove_head, (const resolution_lineage*));
};

using test_dbuct_joint_t = dbuct_joint_elimination_generator<MockConstrainCdcl, MockConstrainMhu>;

struct DbuctJointEliminationGeneratorTest : public ::testing::Test {
    resolution_lineage rl{nullptr, 0};
    NiceMock<MockConstrainCdcl> cdcl;
    NiceMock<MockConstrainMhu> mhu;
    test_dbuct_joint_t joint{cdcl, mhu};
};

TEST_F(DbuctJointEliminationGeneratorTest, CdclYieldRemovesHeadThenAppearsInStream) {
    EXPECT_CALL(cdcl, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));
    EXPECT_CALL(mhu, remove_head(&rl)).Times(1);
    EXPECT_CALL(mhu, constrain(&rl))
        .WillOnce(Return(ByMove(empty_elim_sm())));

    auto sm = joint.constrain(&rl);
    EXPECT_THAT(collect_elims(sm), ElementsAre(&rl));
}

TEST_F(DbuctJointEliminationGeneratorTest, MhuYieldForwardsWithoutRemoveHead) {
    EXPECT_CALL(cdcl, constrain(&rl))
        .WillOnce(Return(ByMove(empty_elim_sm())));
    EXPECT_CALL(mhu, remove_head(::testing::_)).Times(0);
    EXPECT_CALL(mhu, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));

    auto sm = joint.constrain(&rl);
    EXPECT_THAT(collect_elims(sm), ElementsAre(&rl));
}

TEST_F(DbuctJointEliminationGeneratorTest, EmptyStreamsYieldNothing) {
    EXPECT_CALL(cdcl, constrain(&rl))
        .WillOnce(Return(ByMove(empty_elim_sm())));
    EXPECT_CALL(mhu, constrain(&rl))
        .WillOnce(Return(ByMove(empty_elim_sm())));

    auto sm = joint.constrain(&rl);
    EXPECT_THAT(collect_elims(sm), ElementsAre());
    EXPECT_TRUE(sm.done());
}

TEST_F(DbuctJointEliminationGeneratorTest, CdclResultsPrecedeMhuResults) {
    resolution_lineage cdcl_elim{nullptr, 1};
    resolution_lineage mhu_elim{nullptr, 2};

    EXPECT_CALL(cdcl, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&cdcl_elim))));
    EXPECT_CALL(mhu, remove_head(&cdcl_elim)).Times(1);
    EXPECT_CALL(mhu, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&mhu_elim))));

    auto sm = joint.constrain(&rl);
    EXPECT_THAT(collect_elims(sm), ElementsAre(&cdcl_elim, &mhu_elim));
}

}  // namespace
