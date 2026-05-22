#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "../../../core/hpp/infrastructure/joint_elimination_generator.hpp"
#include "../../../core/hpp/interfaces/i_cdcl_elimination_generator.hpp"
#include "../../../core/hpp/interfaces/i_mhu_elimination_generator.hpp"
#include "../../../core/hpp/utility/state_machine.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

const resolution_lineage* sm_cdcl_elim = nullptr;
const resolution_lineage* sm_mhu_elim = nullptr;

state_machine<const resolution_lineage*> make_cdcl_sm() {
    co_yield sm_cdcl_elim;
    co_yield nullptr;
}

state_machine<const resolution_lineage*> make_mhu_sm() {
    co_yield sm_mhu_elim;
    co_yield nullptr;
}

std::vector<const resolution_lineage*> collect_elims(
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

struct MockCdclEliminationGenerator : public i_cdcl_elimination_generator {
    MOCK_METHOD(const resolution_lineage*, learn, (const lemma&), (override));
    MOCK_METHOD(state_machine<const resolution_lineage*>, constrain,
        (const resolution_lineage*), (override));
};

struct MockMhuEliminationGenerator : public i_mhu_elimination_generator {
    MOCK_METHOD(void, add_head,
        (const resolution_lineage*, unify_head, const std::unordered_set<uint32_t>&),
        (override));
    MOCK_METHOD(void, try_remove_head, (const resolution_lineage*), (override));
    MOCK_METHOD(state_machine<const resolution_lineage*>, constrain,
        (const resolution_lineage*), (override));
};

struct JointEliminationGeneratorUnitTest : public ::testing::Test {
protected:
    expr head{expr::var{0}};
    rule r{&head, {}};
    resolution_lineage rl{nullptr, &r};

    NiceMock<MockCdclEliminationGenerator> cdcl;
    NiceMock<MockMhuEliminationGenerator> mhu;
    joint_elimination_generator joint{cdcl, mhu};
};

TEST_F(JointEliminationGeneratorUnitTest, ConstrainYieldsCdclThenMhuThenNullTerminator) {
    sm_cdcl_elim = &rl;
    sm_mhu_elim = &rl;

    EXPECT_CALL(cdcl, constrain(&rl)).WillOnce(Return(make_cdcl_sm()));
    EXPECT_CALL(mhu, constrain(&rl)).WillOnce(Return(make_mhu_sm()));

    auto sm = joint.constrain(&rl);
    auto elims = collect_elims(sm);

    ASSERT_EQ(elims.size(), 2u);
    EXPECT_EQ(elims[0], &rl);
    EXPECT_EQ(elims[1], &rl);
}

TEST_F(JointEliminationGeneratorUnitTest, ConstrainRunsCdclBeforeMhu) {
    testing::InSequence seq;

    EXPECT_CALL(cdcl, constrain(&rl)).WillOnce(Return(make_cdcl_sm()));
    EXPECT_CALL(mhu, constrain(&rl)).WillOnce(Return(make_mhu_sm()));

    auto sm = joint.constrain(&rl);
    collect_elims(sm);
}

TEST_F(JointEliminationGeneratorUnitTest, ConstrainEndsWithNullTerminatorYield) {
    EXPECT_CALL(cdcl, constrain(&rl)).WillOnce(Return(make_cdcl_sm()));
    EXPECT_CALL(mhu, constrain(&rl)).WillOnce(Return(make_mhu_sm()));

    auto sm = joint.constrain(&rl);
    const resolution_lineage* last = nullptr;
    while (!sm.done()) {
        auto v = sm.resume();
        if (v.has_value())
            last = v.value();
    }
    EXPECT_EQ(last, nullptr);
}
