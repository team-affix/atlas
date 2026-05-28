// joint_elimination_generator composes CDCL and MHU elimination streams. These unit
// tests mock elimination via i_elimination_generator (GMock-friendly) behind thin
// i_cdcl / i_mhu facades, and assert constrain() ordering and null termination.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "../../../core/hpp/infrastructure/joint_elimination_generator.hpp"
#include "../../../core/hpp/interfaces/i_elimination_generator.hpp"
#include "../../../core/hpp/utility/state_machine.hpp"

using ::testing::ByMove;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::Return;

namespace {

state_machine<const resolution_lineage*> make_single_elim_sm(
    const resolution_lineage* elim) {
    co_yield elim;
    co_yield nullptr;
}

std::vector<const resolution_lineage*> collect_non_null_elims(
    state_machine<const resolution_lineage*>& sm) {
    std::vector<const resolution_lineage*> out;
    while (!sm.done()) {
        auto v = sm.resume();
        if (v.has_value() && v.value() != nullptr)
            out.push_back(v.value());
    }
    return out;
}

} // namespace

struct MockEliminationGenerator : public i_elimination_generator {
    MOCK_METHOD(state_machine<const resolution_lineage*>, constrain,
        (const resolution_lineage*), (override));
};

struct JointEliminationGeneratorUnitTest : public ::testing::Test {
    expr head{expr::var{0}};
    rule r{&head, {}};
    resolution_lineage rl{nullptr, 0};

    MockEliminationGenerator cdcl_elims;
    MockEliminationGenerator mhu_elims;
    joint_elimination_generator joint{cdcl_elims, mhu_elims};
};

TEST_F(JointEliminationGeneratorUnitTest, ConstrainYieldsCdclThenMhuThenNullTerminator) {
    EXPECT_CALL(cdcl_elims, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));
    EXPECT_CALL(mhu_elims, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));

    auto sm = joint.constrain(&rl);
    EXPECT_THAT(collect_non_null_elims(sm), ElementsAre(&rl, &rl));
}

TEST_F(JointEliminationGeneratorUnitTest, ConstrainRunsCdclBeforeMhu) {
    InSequence seq;

    EXPECT_CALL(cdcl_elims, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));
    EXPECT_CALL(mhu_elims, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));

    auto sm = joint.constrain(&rl);
    collect_non_null_elims(sm);
}

TEST_F(JointEliminationGeneratorUnitTest, ConstrainEndsWithNullTerminatorYield) {
    EXPECT_CALL(cdcl_elims, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));
    EXPECT_CALL(mhu_elims, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));

    auto sm = joint.constrain(&rl);
    const resolution_lineage* last = nullptr;
    while (!sm.done()) {
        auto v = sm.resume();
        if (v.has_value())
            last = v.value();
    }
    EXPECT_EQ(last, nullptr);
}
