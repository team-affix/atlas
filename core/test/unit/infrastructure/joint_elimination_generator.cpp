// joint_elimination_generator composes CDCL and MHU elimination streams. These unit
// tests mock elimination via i_elimination_generator and assert constrain() ordering;
// the joint stream ends with co_return (resume() -> std::nullopt), not a null pointer yield.

#include <gtest/gtest.h>
#include "locator_fixture.hpp"
#include <gmock/gmock.h>
#include <optional>
#include <vector>
#include "infrastructure/joint_elimination_generator.hpp"
#include "infrastructure/cdcl_elimination_generator.hpp"
#include "infrastructure/mhu_elimination_generator.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/overlay_bind_map_factory.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/expr_pool.hpp"
#include "interfaces/i_elimination_generator.hpp"
#include "infrastructure/state_machine.hpp"

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

state_machine<const resolution_lineage*> empty_elim_sm() {
    co_return;
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

struct cdcl_shim : cdcl_elimination_generator {
    cdcl_shim(locator& loc, i_elimination_generator& impl)
        : cdcl_elimination_generator(loc), impl_(impl) {}
    state_machine<const resolution_lineage*> constrain(
        const resolution_lineage* rl) override {
        return impl_.constrain(rl);
    }
private:
    i_elimination_generator& impl_;
};

struct mhu_shim : mhu_elimination_generator {
    mhu_shim(locator& loc, i_elimination_generator& impl)
        : mhu_elimination_generator(loc), impl_(impl) {}
    state_machine<const resolution_lineage*> constrain(
        const resolution_lineage* rl) override {
        return impl_.constrain(rl);
    }
private:
    i_elimination_generator& impl_;
};

struct JointEliminationGeneratorUnitTest : public ::testing::Test {
    resolution_lineage rl{nullptr, 0};

    trail trail_;
    bind_map bind_map_;
    bind_map_factory bind_map_factory_;
    overlay_bind_map_factory overlay_bind_map_factory_;
    unifier_factory unifier_factory_;
    lineage_pool lineage_pool_;
    goal_candidate_rules goal_candidate_rules_;
    locator loc;
    MockEliminationGenerator cdcl_mock;
    MockEliminationGenerator mhu_mock;
    std::optional<expr_pool> expr_pool_;
    std::optional<cdcl_shim> cdcl_elims;
    std::optional<mhu_shim> mhu_elims;
    std::optional<joint_elimination_generator> joint;

    JointEliminationGeneratorUnitTest()
        : trail_(),
          bind_map_(),
          bind_map_factory_(),
          overlay_bind_map_factory_(),
          unifier_factory_(),
          lineage_pool_(),
          goal_candidate_rules_() {
        loc.bind_as<i_log_to_current_trail_frame>(trail_);
        loc.bind_as<i_bind_map>(bind_map_);
        loc.bind_as<i_bind_map_factory>(bind_map_factory_);
        loc.bind_as<i_overlay_bind_map_factory>(overlay_bind_map_factory_);
        loc.bind_as<i_unifier_factory>(unifier_factory_);
        loc.bind_as<i_make_resolution_lineage>(lineage_pool_);
        loc.bind_as<i_get_goal_candidate_rule_ids>(goal_candidate_rules_);
        expr_pool_.emplace(loc);
        loc.bind_as<i_make_functor, i_make_var, i_import_expr, i_get_expr_count>(*expr_pool_);
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

TEST_F(JointEliminationGeneratorUnitTest, ConstrainEndsWithDoneNotNullPointerYield) {
    EXPECT_CALL(cdcl_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));
    EXPECT_CALL(mhu_mock, constrain(&rl))
        .WillOnce(Return(ByMove(make_single_elim_sm(&rl))));

    auto sm = joint->constrain(&rl);
    EXPECT_THAT(collect_non_null_elims(sm), ElementsAre(&rl, &rl));
    EXPECT_TRUE(sm.done());
    EXPECT_FALSE(sm.resume().has_value());
}
