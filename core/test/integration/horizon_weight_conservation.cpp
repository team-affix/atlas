#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include "infrastructure/cumulative_grounded_weight.hpp"
#include "infrastructure/goal_weights.hpp"
#include "infrastructure/horizon_goal_activator.hpp"
#include "infrastructure/horizon_goal_deactivator.hpp"
#include "infrastructure/horizon_initial_goal_activator.hpp"
#include "infrastructure/horizon_resolver.hpp"
#include "infrastructure/initial_goal_weight.hpp"
#include "infrastructure/lineage_pool.hpp"
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
#include "infrastructure/get_resolution_rule.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/rule.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

constexpr double kWeightEpsilon = 1e-9;
constexpr double kTotalWeight = 1.0;

struct MockActivateGoalCandidates {
    MOCK_METHOD(bool, activate_goal_candidates, (const goal_lineage*));
};

struct MockDeactivateGoalCandidates {
    MOCK_METHOD(void, deactivate_goal_candidates, (const goal_lineage*));
};

struct MockSetChosenGoalCandidate {
    MOCK_METHOD(void, set, (const goal_lineage*, rule_id));
};

struct MockGetInitialGoalCount {
    MOCK_METHOD(size_t, count, (), (const));
};

struct MockGetInitialGoalExpr {
    MOCK_METHOD(const expr*, get, (size_t), (const));
};

struct MockGetRule {
    MOCK_METHOD(const rule*, get, (rule_id), (const));
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
    const goal_lineage* parent,
    double& sum) {
    try {
        auto child_sm = active.iterate_child_goals(parent);
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
    const srt_active_goals& active) {
    double sum = 0.0;
    auto root_sm = active.iterate_root_goals();
    while (!root_sm.done()) {
        root_sm.resume();
        if (!root_sm.has_yield())
            continue;
        const goal_lineage* gl = root_sm.consume_yield();
        add_active_leaf_weight(weights, active, gl, sum);
        add_child_weights_if_any(weights, active, gl, sum);
    }
    return sum;
}

class HorizonWeightConservationIntegrationTest : public ::testing::Test {
protected:
    using get_resolution_rule_t  = get_resolution_rule<MockGetRule>;
    using make_initial_goal_lineage_t = make_initial_goal_lineage<lineage_pool>;
    using goal_activator_t  = goal_activator<goal_exprs, goal_candidate_rules,
                                  srt_active_goals, candidate_frame_offsets, get_resolution_rule_t>;
    using srt_goal_deactivator_t = srt_goal_deactivator<goal_exprs, goal_candidate_rules>;
    using horizon_goal_activator_t = horizon_goal_activator<goal_activator_t, goal_weights, MockGetRule>;
    using horizon_goal_deactivator_t = horizon_goal_deactivator<srt_goal_deactivator_t, goal_weights>;
    using initial_goal_activator_t = initial_goal_activator<MockGetInitialGoalExpr,
                                        make_initial_goal_lineage_t, goal_exprs, goal_candidate_rules,
                                        srt_active_goals>;
    using horizon_initial_goal_activator_t = horizon_initial_goal_activator<
                                        initial_goal_activator_t, make_initial_goal_lineage_t,
                                        goal_weights, initial_goal_weight>;
    using subgoals_activator_t = subgoals_activator<lineage_pool, horizon_goal_activator_t,
                                      MockGetRule, MockActivateGoalCandidates>;
    using srt_subgoals_activator_t = srt_subgoals_activator<srt_active_goals, subgoals_activator_t>;
    using resolver_t = resolver<horizon_goal_deactivator_t, srt_subgoals_activator_t,
                              MockDeactivateGoalCandidates, MockSetChosenGoalCandidate>;
    using horizon_resolver_t = horizon_resolver<resolver_t, MockGetRule,
                                    goal_weights, cumulative_grounded_weight>;

    lineage_pool lineage_pool_;
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
    NiceMock<MockSetChosenGoalCandidate> set_chosen_goal_candidate;
    NiceMock<MockGetInitialGoalCount> get_initial_goal_count;
    NiceMock<MockGetInitialGoalExpr> get_initial_goal_expr;
    NiceMock<MockGetRule> get_rule;

    std::optional<get_resolution_rule_t>               get_resolution_rule_;
    std::optional<make_initial_goal_lineage_t>             make_initial_goal_lineage_;
    std::optional<goal_activator_t>               goal_activator_;
    std::optional<horizon_goal_activator_t>        horizon_goal_activator_;
    std::optional<srt_goal_deactivator_t>          srt_goal_deactivator_;
    std::optional<horizon_goal_deactivator_t>      horizon_goal_deactivator_;
    std::optional<initial_goal_activator_t>        initial_goal_activator_;
    std::optional<horizon_initial_goal_activator_t> horizon_initial_goal_activator_;
    std::optional<subgoals_activator_t>           subgoals_activator_;
    std::optional<srt_subgoals_activator_t>        srt_subgoals_activator_;
    std::optional<resolver_t>                    resolver_;
    std::optional<horizon_resolver_t>             horizon_resolver_;

    expr f_head{expr::var{0}};
    expr g_head{expr::var{1}};
    expr h_head{expr::var{2}};
    rule expand_rule{&f_head, {&g_head, &h_head}, 3};
    rule ground_g{&g_head, {}, 2};
    rule ground_h{&h_head, {}, 3};

    void SetUp() override {
        ON_CALL(get_initial_goal_count, count()).WillByDefault(Return(1));
        ON_CALL(get_initial_goal_expr, get(0)).WillByDefault(Return(&f_head));
        ON_CALL(activate_goal_candidates, activate_goal_candidates).WillByDefault(Return(true));
        ON_CALL(get_rule, get).WillByDefault([&](rule_id id) -> const rule* {
            if (id == 0) return &expand_rule;
            if (id == 1) return &ground_g;
            if (id == 2) return &ground_h;
            return nullptr;
        });

        get_resolution_rule_.emplace(get_rule);
        make_initial_goal_lineage_.emplace(lineage_pool_);
        goal_activator_.emplace(goal_exprs_, goal_candidate_rules_, srt_active_goals_,
                                candidate_frame_offsets_, *get_resolution_rule_);
        horizon_goal_activator_.emplace(*goal_activator_, goal_weights_, get_rule);
        srt_goal_deactivator_.emplace(goal_exprs_, goal_candidate_rules_);
        horizon_goal_deactivator_.emplace(*srt_goal_deactivator_, goal_weights_);
        initial_goal_activator_.emplace(get_initial_goal_expr, *make_initial_goal_lineage_,
                                        goal_exprs_, goal_candidate_rules_, srt_active_goals_);
        horizon_initial_goal_activator_.emplace(*initial_goal_activator_,
                                                *make_initial_goal_lineage_,
                                                goal_weights_, initial_goal_weight_);
        subgoals_activator_.emplace(lineage_pool_, *horizon_goal_activator_, get_rule,
                                    activate_goal_candidates);
        srt_subgoals_activator_.emplace(srt_active_goals_, *subgoals_activator_);
        resolver_.emplace(*horizon_goal_deactivator_, *srt_subgoals_activator_,
                          deactivate_goal_candidates, set_chosen_goal_candidate);
        horizon_resolver_.emplace(*resolver_, get_rule, goal_weights_,
                                  cumulative_grounded_weight_);
    }

    void expect_conserved() {
        const double active = sum_active_weights(goal_weights_, srt_active_goals_);
        const double cgw = cumulative_grounded_weight_.get();
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
