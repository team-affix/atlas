// joint_elimination_generator composes CDCL and MHU elimination streams. These unit
// tests mock elimination via plain mock types and assert constrain() ordering;
// the joint stream ends with co_return, not a null pointer yield.
// Duplicate yields for the same candidate are forwarded; elimination_router deduplicates.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <vector>
#include "infrastructure/joint_elimination_generator.hpp"
#include "infrastructure/coroutine.hpp"

using ::testing::ByMove;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::Return;

namespace {

coroutine<const resolution_lineage*, void> make_single_elim_sm(
    const resolution_lineage* elim) {
    co_yield elim;
    co_yield nullptr;
}

coroutine<const resolution_lineage*, void> empty_elim_sm() {
    co_return;
}

std::vector<const resolution_lineage*> collect_non_null_elims(
    coroutine<const resolution_lineage*, void>& sm) {
    std::vector<const resolution_lineage*> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield()) {
            const resolution_lineage* v = sm.consume_yield();
            if (v != nullptr)
                out.push_back(v);
        }
    }
    return out;
}

} // namespace

struct MockCdcl {
    MOCK_METHOD((coroutine<const resolution_lineage*, void>), constrain,
        (const resolution_lineage*));
};

struct MockMhu {
    MOCK_METHOD((coroutine<const resolution_lineage*, void>), constrain,
        (const resolution_lineage*));
};

using test_joint_elimination_generator_t = joint_elimination_generator<MockCdcl, MockMhu>;

struct JointEliminationGeneratorUnitTest : public ::testing::Test {
    resolution_lineage rl{nullptr, 0};
    MockCdcl cdcl_mock;
    MockMhu mhu_mock;
    test_joint_elimination_generator_t joint{cdcl_mock, mhu_mock};
};

TEST_F(JointEliminationGeneratorUnitTest, ConstrainYieldsCdclThenMhu) {
    EXPECT_CALL(cdcl_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));
    EXPECT_CALL(mhu_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));

    auto sm = joint.constrain(&rl);
    EXPECT_THAT(collect_non_null_elims(sm), ElementsAre(&rl, &rl));
}

TEST_F(JointEliminationGeneratorUnitTest, ConstrainRunsCdclBeforeMhu) {
    InSequence seq;

    EXPECT_CALL(cdcl_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));
    EXPECT_CALL(mhu_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));

    auto sm = joint.constrain(&rl);
    collect_non_null_elims(sm);
}

TEST_F(JointEliminationGeneratorUnitTest, ConstrainYieldsOnlyMhuWhenCdclEmpty) {
    EXPECT_CALL(cdcl_mock, constrain(&rl))
        .WillOnce(Return(ByMove(empty_elim_sm())));
    EXPECT_CALL(mhu_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));

    auto sm = joint.constrain(&rl);
    EXPECT_THAT(collect_non_null_elims(sm), ElementsAre(&rl));
}

TEST_F(JointEliminationGeneratorUnitTest, ConstrainYieldsNothingWhenBothStreamsEmpty) {
    EXPECT_CALL(cdcl_mock, constrain(&rl))
        .WillOnce(Return(ByMove(empty_elim_sm())));
    EXPECT_CALL(mhu_mock, constrain(&rl))
        .WillOnce(Return(ByMove(empty_elim_sm())));

    auto sm = joint.constrain(&rl);
    EXPECT_THAT(collect_non_null_elims(sm), ElementsAre());
    EXPECT_TRUE(sm.done());
}

TEST_F(JointEliminationGeneratorUnitTest, ConstrainEndsWithDoneNotNullPointerYield) {
    EXPECT_CALL(cdcl_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));
    EXPECT_CALL(mhu_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));

    auto sm = joint.constrain(&rl);
    EXPECT_THAT(collect_non_null_elims(sm), ElementsAre(&rl, &rl));
    EXPECT_TRUE(sm.done());
}
