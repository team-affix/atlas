// Integration: horizon weight conservation — active goal weights plus CGW sum to 1.0
// across initial activation, expansion, and fact grounding.

#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "infrastructure/cumulative_grounded_weight.hpp"
#include "infrastructure/goal_weights.hpp"
#include "infrastructure/horizon_goal_activator.hpp"
#include "infrastructure/horizon_goal_deactivator.hpp"
#include "infrastructure/horizon_initial_goal_activator.hpp"
#include "infrastructure/horizon_resolver.hpp"
#include "infrastructure/initial_goal_weight.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/locator.hpp"
#include "infrastructure/resolver.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "infrastructure/srt_goal_deactivator.hpp"
#include "infrastructure/srt_subgoals_activator.hpp"
#include "infrastructure/subgoals_activator.hpp"
#include "infrastructure/goal_activator.hpp"
#include "infrastructure/goal_exprs.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/initial_goal_activator.hpp"
#include "infrastructure/make_initial_goal_lineage.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/candidate_frame_offsets.hpp"
#include "infrastructure/frame_bump_allocator.hpp"
#include "infrastructure/get_resolution_rule.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/rule_id_set_factory.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/expr_pool.hpp"
#include "interfaces/i_frame_allocator.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"
#include "interfaces/i_import_expr.hpp"
#include "interfaces/i_get_expr_count.hpp"
#include "interfaces/i_activate_goal_candidates.hpp"
#include "interfaces/i_deactivate_goal_candidates.hpp"
#include "interfaces/i_get_initial_goal_count.hpp"
#include "interfaces/i_get_initial_goal_expr.hpp"
#include "interfaces/i_get_rule.hpp"
#include "interfaces/i_get_grounded_weight.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/rule.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

constexpr double kWeightEpsilon = 1e-9;
constexpr double kTotalWeight = 1.0;

struct MockActivateGoalCandidates : public i_activate_goal_candidates {
    MOCK_METHOD(bool, activate_goal_candidates, (const goal_lineage*), (override));
};

struct MockDeactivateGoalCandidates : public i_deactivate_goal_candidates {
    MOCK_METHOD(void, deactivate_goal_candidates, (const goal_lineage*), (override));
};

struct MockGetInitialGoalCount : public i_get_initial_goal_count {
    MOCK_METHOD(size_t, count, (), (const, override));
};

struct MockGetInitialGoalExpr : public i_get_initial_goal_expr {
    MOCK_METHOD(const expr*, get, (size_t), (const, override));
};

struct MockGetRule : public i_get_rule {
    MOCK_METHOD(const rule*, get, (rule_id), (const, override));
};

void add_active_leaf_weight(
    const goal_weights& weights,
    const srt_active_goals& active,
    const goal_lineage* gl,
    double& sum) {
    if (active.is_active_goal(gl))
        sum += weights.get(gl);
}

void add_child_weights_if_any(
    const goal_weights& weights,
    const srt_active_goals& active,
    i_iterate_child_goals& children,
    const goal_lineage* parent,
    double& sum) {
    try {
        auto child_sm = children.iterate_child_goals(parent);
        while (!child_sm.done()) {
            child_sm.resume();
            if (!child_sm.has_yield())
                continue;
            add_active_leaf_weight(weights, active, child_sm.consume_yield(), sum);
        }
    } catch (const std::out_of_range&) {
    }
}

double sum_active_weights(
    const goal_weights& weights,
    const srt_active_goals& active,
    i_iterate_root_goals& roots,
    i_iterate_child_goals& children) {
    double sum = 0.0;
    auto root_sm = roots.iterate_root_goals();
    while (!root_sm.done()) {
        root_sm.resume();
        if (!root_sm.has_yield())
            continue;
        const goal_lineage* gl = root_sm.consume_yield();
        add_active_leaf_weight(weights, active, gl, sum);
        add_child_weights_if_any(weights, active, children, gl, sum);
    }
    return sum;
}

class HorizonWeightConservationIntegrationTest : public ::testing::Test {
protected:
    locator loc;
    trail trail_;
    globalizer globalizer_;
    bind_map bind_map_{globalizer_};
    bind_map_factory bind_map_factory_{globalizer_};
    unifier_factory unifier_factory_{loc};
    lineage_pool lineage_pool_;
    std::unique_ptr<expr_pool> expr_pool_;
    std::unique_ptr<frame_bump_allocator> frame_allocator_;
    rule_id_set_factory rule_id_set_factory_;
    ra_rule_id_set_factory ra_rule_id_set_factory_;
    srt_active_goals srt_active_goals_;
    goal_exprs goal_exprs_;
    goal_weights goal_weights_;
    cumulative_grounded_weight cumulative_grounded_weight_;
    initial_goal_weight initial_goal_weight_{kTotalWeight};
    goal_candidate_rules goal_candidate_rules_{ra_rule_id_set_factory_};
    candidate_frame_offsets candidate_frame_offsets_;
    NiceMock<MockActivateGoalCandidates> activate_goal_candidates;
    NiceMock<MockDeactivateGoalCandidates> deactivate_goal_candidates;
    NiceMock<MockGetInitialGoalCount> get_initial_goal_count;
    NiceMock<MockGetInitialGoalExpr> get_initial_goal_expr;
    NiceMock<MockGetRule> get_rule;

    std::unique_ptr<make_initial_goal_lineage> make_initial_goal_lineage_;
    std::unique_ptr<get_resolution_rule> get_resolution_rule_;
    std::unique_ptr<goal_activator> goal_activator_;
    std::unique_ptr<horizon_goal_activator> horizon_goal_activator_;
    std::unique_ptr<srt_goal_deactivator> srt_goal_deactivator_;
    std::unique_ptr<horizon_goal_deactivator> horizon_goal_deactivator_;
    std::unique_ptr<initial_goal_activator> initial_goal_activator_;
    std::unique_ptr<horizon_initial_goal_activator> horizon_initial_goal_activator_;
    std::unique_ptr<subgoals_activator> subgoals_activator_;
    std::unique_ptr<srt_subgoals_activator> srt_subgoals_activator_;
    std::unique_ptr<resolver> resolver_;
    std::unique_ptr<horizon_resolver> horizon_resolver_;

    expr f_head{expr::var{0}};
    expr g_head{expr::var{1}};
    expr h_head{expr::var{2}};
    rule expand_rule{&f_head, {&g_head, &h_head}, 3};
    rule ground_g{&g_head, {}, 2};
    rule ground_h{&h_head, {}, 3};

    void SetUp() override {
        loc.bind_as<i_globalizer>(globalizer_);
        loc.bind_as<i_log_to_current_trail_frame>(trail_);
        loc.bind_as<i_bind_map>(bind_map_);
        loc.bind_as<i_bind_map_factory>(bind_map_factory_);
        loc.bind_as<i_unifier_factory>(unifier_factory_);
        loc.bind_as<i_make_goal_lineage, i_make_resolution_lineage>(lineage_pool_);
        expr_pool_ = std::make_unique<expr_pool>();
        loc.bind_as<i_make_functor, i_make_var, i_import_expr, i_get_expr_count>(*expr_pool_);
        frame_allocator_ = std::make_unique<frame_bump_allocator>(0u);
        loc.bind_as<i_frame_allocator>(*frame_allocator_);
        loc.bind_as<i_db_rule_id_set_factory>(rule_id_set_factory_);
        loc.bind_as<i_candidate_rule_id_set_factory>(ra_rule_id_set_factory_);
        loc.bind_as<i_insert_active_goal, i_is_active_goal,
            i_iterate_root_goals, i_iterate_child_goals,
            i_active_goals_size, i_check_active_goals_empty, i_clear_active_goals,
            i_srt_link_goal_batch_parent, i_srt_flush_goal_batch>(srt_active_goals_);
        loc.bind_as<i_get_goal_expr, i_set_goal_expr, i_unset_goal_expr>(goal_exprs_);
        loc.bind_as<i_get_goal_weight, i_set_goal_weight, i_erase_goal_weight>(goal_weights_);
        loc.bind_as<i_accumulate_grounded_weight, i_get_grounded_weight>(cumulative_grounded_weight_);
        loc.bind_as<i_get_initial_goal_weight>(initial_goal_weight_);
        loc.bind_as<i_get_goal_candidate_rule_ids, i_insert_goal_candidates,
            i_erase_goal_candidates>(goal_candidate_rules_);
        loc.bind_as<i_get_candidate_frame_offset, i_set_candidate_frame_offset>(candidate_frame_offsets_);
        loc.bind_as<i_get_initial_goal_count>(get_initial_goal_count);
        loc.bind_as<i_get_initial_goal_expr>(get_initial_goal_expr);
        loc.bind_as<i_get_rule>(get_rule);
        loc.bind_as<i_activate_goal_candidates>(activate_goal_candidates);
        loc.bind_as<i_deactivate_goal_candidates>(deactivate_goal_candidates);

        ON_CALL(get_initial_goal_count, count()).WillByDefault(Return(1));
        ON_CALL(get_initial_goal_expr, get(0)).WillByDefault(Return(&f_head));
        ON_CALL(activate_goal_candidates, activate_goal_candidates).WillByDefault(Return(true));
        ON_CALL(get_rule, get).WillByDefault([&](rule_id id) -> const rule* {
            if (id == 0)
                return &expand_rule;
            if (id == 1)
                return &ground_g;
            if (id == 2)
                return &ground_h;
            return nullptr;
        });

        get_resolution_rule_ = std::make_unique<get_resolution_rule>(loc);
        loc.bind_as<i_get_resolution_rule>(*get_resolution_rule_);
        make_initial_goal_lineage_ = std::make_unique<make_initial_goal_lineage>(loc);
        loc.bind_as<i_make_initial_goal_lineage>(*make_initial_goal_lineage_);

        goal_activator_ = std::make_unique<goal_activator>(loc);
        loc.bind_as<>(*goal_activator_);
        horizon_goal_activator_ = std::make_unique<horizon_goal_activator>(loc);
        loc.bind_as<i_goal_activator>(*horizon_goal_activator_);

        srt_goal_deactivator_ = std::make_unique<srt_goal_deactivator>(loc);
        loc.bind_as<>(*srt_goal_deactivator_);
        horizon_goal_deactivator_ = std::make_unique<horizon_goal_deactivator>(loc);
        loc.bind_as<i_goal_deactivator>(*horizon_goal_deactivator_);

        initial_goal_activator_ = std::make_unique<initial_goal_activator>(loc);
        loc.bind_as<>(*initial_goal_activator_);
        horizon_initial_goal_activator_ = std::make_unique<horizon_initial_goal_activator>(loc);
        loc.bind_as<i_activate_initial_goal>(*horizon_initial_goal_activator_);

        subgoals_activator_ = std::make_unique<subgoals_activator>(loc);
        loc.bind_as<>(*subgoals_activator_);
        srt_subgoals_activator_ = std::make_unique<srt_subgoals_activator>(loc);
        loc.bind_as<i_activate_subgoals_and_candidates>(*srt_subgoals_activator_);

        resolver_ = std::make_unique<resolver>(loc);
        loc.bind_as<>(*resolver_);
        horizon_resolver_ = std::make_unique<horizon_resolver>(loc);
        loc.bind_as<i_resolver>(*horizon_resolver_);
    }

    void expect_conserved() {
        const double active = sum_active_weights(
            goal_weights_,
            srt_active_goals_,
            loc.locate<i_iterate_root_goals>(),
            loc.locate<i_iterate_child_goals>());
        const double cgw = loc.locate<i_get_grounded_weight>().get();
        EXPECT_NEAR(active + cgw, kTotalWeight, kWeightEpsilon);
    }
};

TEST_F(HorizonWeightConservationIntegrationTest, InitialActivationConservesTotalWeight) {
    horizon_initial_goal_activator_->activate_initial_goal(0);
    srt_active_goals_.flush_srt_goal_batch();
    expect_conserved();
    EXPECT_NEAR(goal_weights_.get(make_initial_goal_lineage_->make(0)), kTotalWeight, kWeightEpsilon);
    EXPECT_DOUBLE_EQ(cumulative_grounded_weight_.get(), 0.0);
}

TEST_F(HorizonWeightConservationIntegrationTest, ExpandThenGroundChildrenConservesTotalWeight) {
    horizon_initial_goal_activator_->activate_initial_goal(0);
    srt_active_goals_.flush_srt_goal_batch();
    const goal_lineage* root = make_initial_goal_lineage_->make(0);
    const resolution_lineage* expand_rl = lineage_pool_.make_resolution_lineage(root, 0);
    // Register frame offsets for manually-created candidates (bypass candidate_activator).
    candidate_frame_offsets_.set(expand_rl, 0u);
    ASSERT_TRUE(srt_subgoals_activator_->activate_subgoals_and_candidates(expand_rl));
    horizon_goal_deactivator_->deactivate(root);
    expect_conserved();

    const resolution_lineage* g_rl = lineage_pool_.make_resolution_lineage(
        lineage_pool_.make_goal_lineage(expand_rl, 0), 1);
    const resolution_lineage* h_rl = lineage_pool_.make_resolution_lineage(
        lineage_pool_.make_goal_lineage(expand_rl, 1), 2);
    candidate_frame_offsets_.set(g_rl, 0u);
    ASSERT_TRUE(horizon_resolver_->resolve(g_rl));
    expect_conserved();
    EXPECT_NEAR(cumulative_grounded_weight_.get(), kTotalWeight / 2.0, kWeightEpsilon);

    candidate_frame_offsets_.set(h_rl, 0u);
    ASSERT_TRUE(horizon_resolver_->resolve(h_rl));
    expect_conserved();
    EXPECT_NEAR(cumulative_grounded_weight_.get(), kTotalWeight, kWeightEpsilon);
}

}  // namespace
