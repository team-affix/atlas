// Integration: remaining work conserved across quell initial activate + fact resolve.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/goal_depths.hpp"
#include "infrastructure/goal_work_function.hpp"
#include "infrastructure/goal_work_values.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/make_initial_goal_lineage.hpp"
#include "infrastructure/quell_initial_goal_activator.hpp"
#include "infrastructure/quell_resolver.hpp"
#include "infrastructure/remaining_work.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/rule.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

constexpr double kWorkEpsilon = 1e-9;
constexpr double kWorkDecayK = 0.2;
constexpr double kWorkDecayJ = 10.0;

struct MockInitialGoalActivator {
    MOCK_METHOD(void, activate_initial_goal, (subgoal_id));
};

struct MockResolver {
    MOCK_METHOD(bool, resolve, (const resolution_lineage*));
};

struct MockGetRule {
    MOCK_METHOD(const rule*, get_rule, (rule_id), (const));
};

class QuellWorkConservationIntegrationTest : public ::testing::Test {
protected:
    using make_initial_goal_lineage_t = make_initial_goal_lineage<lineage_pool>;
    using quell_initial_goal_activator_t = quell_initial_goal_activator<
        MockInitialGoalActivator, make_initial_goal_lineage_t,
        goal_depths, goal_work_values, goal_work_function, remaining_work>;
    using quell_resolver_t = quell_resolver<
        MockResolver, MockGetRule, goal_work_values, goal_depths, goal_work_function,
        remaining_work, remaining_work>;

    lineage_pool lineage_pool_;
    goal_depths goal_depths_;
    goal_work_values goal_work_values_;
    remaining_work remaining_work_;
    goal_work_function goal_work_function_{kWorkDecayK, kWorkDecayJ};
    make_initial_goal_lineage_t make_initial_goal_lineage_{lineage_pool_};
    NiceMock<MockInitialGoalActivator> mock_initial;
    NiceMock<MockResolver> mock_resolver;
    NiceMock<MockGetRule> get_rule;

    quell_initial_goal_activator_t quell_initial_goal_activator_{
        mock_initial, make_initial_goal_lineage_, goal_depths_, goal_work_values_,
        goal_work_function_, remaining_work_};
    quell_resolver_t quell_resolver_{
        mock_resolver, get_rule, goal_work_values_, goal_depths_, goal_work_function_,
        remaining_work_, remaining_work_};

    expr head{expr::var{0}};
    rule fact_rule{&head, {}};

    double f0() const { return goal_work_function_.get(0); }
};

TEST_F(QuellWorkConservationIntegrationTest, TwoInitialGoalsThenFactResolvesConserveRemaining) {
    ON_CALL(get_rule, get_rule).WillByDefault(Return(&fact_rule));
    ON_CALL(mock_resolver, resolve).WillByDefault(Return(true));

    quell_initial_goal_activator_.activate_initial_goal(0);
    quell_initial_goal_activator_.activate_initial_goal(1);
    EXPECT_NEAR(remaining_work_.get(), 2.0 * f0(), kWorkEpsilon);

    const goal_lineage* gl0 = make_initial_goal_lineage_.make(0);
    const goal_lineage* gl1 = make_initial_goal_lineage_.make(1);
    const resolution_lineage* rl0 = lineage_pool_.make_resolution_lineage(gl0, 0);
    const resolution_lineage* rl1 = lineage_pool_.make_resolution_lineage(gl1, 1);

    ASSERT_TRUE(quell_resolver_.resolve(rl0));
    EXPECT_NEAR(remaining_work_.get(), f0(), kWorkEpsilon);

    ASSERT_TRUE(quell_resolver_.resolve(rl1));
    EXPECT_NEAR(remaining_work_.get(), 0.0, kWorkEpsilon);
}

}  // namespace
