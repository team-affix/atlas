// joint_elimination_generator composes CDCL and MHU elimination streams. These unit
// tests mock elimination via i_elimination_generator and assert constrain() ordering;
// the joint stream ends with co_return, not a null pointer yield.
// Duplicate yields for the same candidate are forwarded; elimination_router deduplicates.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <vector>
#include "locator_fixture.hpp"
#include "infrastructure/joint_elimination_generator.hpp"
#include "infrastructure/cdcl_elimination_generator.hpp"
#include "infrastructure/cdcl_sequencer.hpp"
#include "infrastructure/mhu_elimination_generator.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/expr_pool.hpp"
#include "interfaces/i_elimination_generator.hpp"
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

struct MockEliminationGenerator : public i_elimination_generator {
    MOCK_METHOD((coroutine<const resolution_lineage*, void>), constrain,
        (const resolution_lineage*), (override));
};

struct cdcl_shim : cdcl_elimination_generator {
    cdcl_shim(locator& loc, i_elimination_generator& impl)
        : cdcl_elimination_generator(loc), impl_(impl) {}
    coroutine<const resolution_lineage*, void> constrain(
        const resolution_lineage* rl) override {
        return impl_.constrain(rl);
    }
private:
    i_elimination_generator& impl_;
};

struct mhu_shim : mhu_elimination_generator {
    mhu_shim(locator& loc, i_elimination_generator& impl)
        : mhu_elimination_generator(loc), impl_(impl) {}
    coroutine<const resolution_lineage*, void> constrain(
        const resolution_lineage* rl) override {
        return impl_.constrain(rl);
    }
private:
    i_elimination_generator& impl_;
};

struct JointEliminationGeneratorUnitTest : public ::testing::Test {
    resolution_lineage rl{nullptr, 0};

    trail trail_;
    globalizer globalizer_;
    bind_map bind_map_{globalizer_};
    bind_map_factory bind_map_factory_{globalizer_};
    unifier_factory unifier_factory_{loc};
    lineage_pool lineage_pool_;
    ra_rule_id_set_factory ra_rule_id_set_factory_;
    goal_candidate_rules goal_candidate_rules_;
    locator loc;
    MockEliminationGenerator cdcl_mock;
    MockEliminationGenerator mhu_mock;
    std::optional<expr_pool> expr_pool_;
    std::optional<cdcl_sequencer> cdcl_seq_;
    std::optional<cdcl_shim> cdcl_elims;
    std::optional<mhu_shim> mhu_elims;
    std::optional<joint_elimination_generator> joint;

    JointEliminationGeneratorUnitTest()
        : trail_(),
          globalizer_(),
          bind_map_(globalizer_),
          bind_map_factory_(globalizer_),
          unifier_factory_(loc),
          lineage_pool_(),
          ra_rule_id_set_factory_(),
          goal_candidate_rules_(ra_rule_id_set_factory_),
          loc() {
        loc.bind_as<i_globalizer>(globalizer_);
        loc.bind_as<i_log_to_current_trail_frame>(trail_);
        loc.bind_as<i_bind_map>(bind_map_);
        loc.bind_as<i_bind_map_factory>(bind_map_factory_);
        loc.bind_as<i_unifier_factory>(unifier_factory_);
        loc.bind_as<i_make_resolution_lineage>(lineage_pool_);
        loc.bind_as<i_get_goal_candidate_rule_ids>(goal_candidate_rules_);
        expr_pool_.emplace();
        loc.bind_as<i_make_functor, i_make_var, i_import_expr, i_get_expr_count>(*expr_pool_);
        cdcl_seq_.emplace(loc);
        loc.bind_as<i_cdcl_sequencer>(*cdcl_seq_);
        cdcl_elims.emplace(loc, cdcl_mock);
        mhu_elims.emplace(loc, mhu_mock);
        loc.bind_as<cdcl_elimination_generator>(*cdcl_elims);
        loc.bind_as<mhu_elimination_generator>(*mhu_elims);
        joint.emplace(loc);
    }
};

TEST_F(JointEliminationGeneratorUnitTest, ConstrainYieldsCdclThenMhu) {
    EXPECT_CALL(cdcl_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));
    EXPECT_CALL(mhu_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));

    auto sm = joint->constrain(&rl);
    EXPECT_THAT(collect_non_null_elims(sm), ElementsAre(&rl, &rl));
}

TEST_F(JointEliminationGeneratorUnitTest, ConstrainRunsCdclBeforeMhu) {
    InSequence seq;

    EXPECT_CALL(cdcl_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));
    EXPECT_CALL(mhu_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));

    auto sm = joint->constrain(&rl);
    collect_non_null_elims(sm);
}

TEST_F(JointEliminationGeneratorUnitTest, ConstrainYieldsOnlyMhuWhenCdclEmpty) {
    EXPECT_CALL(cdcl_mock, constrain(&rl))
        .WillOnce(Return(ByMove(empty_elim_sm())));
    EXPECT_CALL(mhu_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));

    auto sm = joint->constrain(&rl);
    EXPECT_THAT(collect_non_null_elims(sm), ElementsAre(&rl));
}

TEST_F(JointEliminationGeneratorUnitTest, ConstrainYieldsNothingWhenBothStreamsEmpty) {
    EXPECT_CALL(cdcl_mock, constrain(&rl))
        .WillOnce(Return(ByMove(empty_elim_sm())));
    EXPECT_CALL(mhu_mock, constrain(&rl))
        .WillOnce(Return(ByMove(empty_elim_sm())));

    auto sm = joint->constrain(&rl);
    EXPECT_THAT(collect_non_null_elims(sm), ElementsAre());
    EXPECT_TRUE(sm.done());
}

TEST_F(JointEliminationGeneratorUnitTest, ConstrainEndsWithDoneNotNullPointerYield) {
    EXPECT_CALL(cdcl_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));
    EXPECT_CALL(mhu_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));

    auto sm = joint->constrain(&rl);
    EXPECT_THAT(collect_non_null_elims(sm), ElementsAre(&rl, &rl));
    EXPECT_TRUE(sm.done());
}
