// Integration: remaining work conserved across quell initial activate + fact resolve.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/goal_depths.hpp"
#include "infrastructure/goal_work_function.hpp"
#include "infrastructure/goal_work_values.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/make_initial_goal_lineage.hpp"
#include "infrastructure/quell_goal_activator.hpp"
#include "infrastructure/quell_initial_goal_activator.hpp"
#include "infrastructure/quell_resolver.hpp"
#include "infrastructure/remaining_work.hpp"
#include "infrastructure/resolver.hpp"
#include "value_objects/expr.hpp"

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

struct MockGoalActivator {
    MOCK_METHOD(void, activate, (const goal_lineage*));
};

struct MockActivateSubgoalsAndCandidates {
    MOCK_METHOD(bool, activate_subgoals_and_candidates, (const resolution_lineage*));
};

struct MockGoalDeactivator {
    MOCK_METHOD(void, deactivate, (const goal_lineage*));
};

struct MockDeactivateGoalCandidates {
    MOCK_METHOD(void, deactivate_goal_candidates, (const goal_lineage*));
};

struct MockSetChosenGoalCandidate {
    MOCK_METHOD(void, set, (const goal_lineage*, rule_id));
};

class QuellWorkConservationIntegrationTest : public ::testing::Test {
protected:
    using make_initial_goal_lineage_t = make_initial_goal_lineage<lineage_pool>;
    using quell_initial_goal_activator_t = quell_initial_goal_activator<
        MockInitialGoalActivator, make_initial_goal_lineage_t,
        goal_depths, goal_work_values, goal_work_function, remaining_work>;
    using quell_resolver_t = quell_resolver<
        MockResolver, goal_work_values, remaining_work>;
    using quell_goal_activator_t = quell_goal_activator<
        MockGoalActivator, goal_depths, goal_depths, goal_work_values,
        goal_work_function, remaining_work>;
    // The inner resolver is the real one, so the failure path exercises its own
    // early return rather than a mock that merely reports false.
    using inner_resolver_t = resolver<
        MockGoalDeactivator, MockActivateSubgoalsAndCandidates,
        MockDeactivateGoalCandidates, MockSetChosenGoalCandidate>;
    using quell_real_resolver_t = quell_resolver<
        inner_resolver_t, goal_work_values, remaining_work>;

    lineage_pool lineage_pool_;
    goal_depths goal_depths_;
    goal_work_values goal_work_values_;
    remaining_work remaining_work_;
    goal_work_function goal_work_function_{kWorkDecayK, kWorkDecayJ};
    make_initial_goal_lineage_t make_initial_goal_lineage_{lineage_pool_};
    NiceMock<MockInitialGoalActivator> mock_initial;
    NiceMock<MockResolver> mock_resolver;
    NiceMock<MockGoalActivator> mock_goal_activator;
    NiceMock<MockActivateSubgoalsAndCandidates> mock_activate_subgoals;
    NiceMock<MockGoalDeactivator> mock_goal_deactivator;
    NiceMock<MockDeactivateGoalCandidates> mock_deactivate_candidates;
    NiceMock<MockSetChosenGoalCandidate> mock_set_chosen;

    quell_initial_goal_activator_t quell_initial_goal_activator_{
        mock_initial, make_initial_goal_lineage_, goal_depths_, goal_work_values_,
        goal_work_function_, remaining_work_};
    quell_resolver_t quell_resolver_{
        mock_resolver, goal_work_values_, remaining_work_};
    quell_goal_activator_t quell_goal_activator_{
        mock_goal_activator, goal_depths_, goal_depths_, goal_work_values_,
        goal_work_function_, remaining_work_};
    inner_resolver_t inner_resolver_{
        mock_goal_deactivator, mock_activate_subgoals, mock_deactivate_candidates,
        mock_set_chosen};
    quell_real_resolver_t quell_real_resolver_{
        inner_resolver_, goal_work_values_, remaining_work_};

    double f(size_t depth) const { return goal_work_function_.get(depth); }
    double f0() const { return goal_work_function_.get(0); }

    // Activates the body goals of rl at depth(parent) + 1 the way
    // subgoals_activator does, and returns them.
    std::vector<const goal_lineage*> activate_children(
        const resolution_lineage* rl, size_t body_size) {
        std::vector<const goal_lineage*> children;
        for (size_t i = 0; i < body_size; ++i) {
            const goal_lineage* child =
                lineage_pool_.make_goal_lineage(rl, static_cast<subgoal_id>(i));
            quell_goal_activator_.activate(child);
            children.push_back(child);
        }
        return children;
    }

    double sum_work(const std::vector<const goal_lineage*>& goals) const {
        double sum = 0.0;
        for (const goal_lineage* gl : goals)
            sum += goal_work_values_.get(gl);
        return sum;
    }
};

TEST_F(QuellWorkConservationIntegrationTest, TwoInitialGoalsThenFactResolvesConserveRemaining) {
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

TEST_F(QuellWorkConservationIntegrationTest, ChildActivationAtDepthOneAddsWorkAtDepthOne) {
    // quell_goal_activator is the only path that assigns a non-zero depth, and no
    // conservation test has ever run it: everything so far stops at initial goals.
    quell_initial_goal_activator_.activate_initial_goal(0);
    const goal_lineage* root = make_initial_goal_lineage_.make(0);
    const resolution_lineage* rl = lineage_pool_.make_resolution_lineage(root, 0);

    const std::vector<const goal_lineage*> children = activate_children(rl, 2);

    EXPECT_EQ(goal_depths_.get(children[0]), 1u);
    EXPECT_EQ(goal_depths_.get(children[1]), 1u);
    EXPECT_NEAR(goal_work_values_.get(children[0]), f(1), kWorkEpsilon);
    EXPECT_NEAR(goal_work_values_.get(children[1]), f(1), kWorkEpsilon);
    // Children are credited before the parent is debited, so all three are live.
    EXPECT_NEAR(remaining_work_.get(), f(0) + 2.0 * f(1), kWorkEpsilon);
}

TEST_F(QuellWorkConservationIntegrationTest, SumOfGoalWorkValuesEqualsRemainingWorkAfterMixedDepths) {
    // The invariant the quell reward depends on: remaining_work is exactly the
    // total work of the live frontier, at whatever mix of depths it spans. Drift
    // here silently re-weights the entire MCTS search.
    ON_CALL(mock_resolver, resolve).WillByDefault(Return(true));

    quell_initial_goal_activator_.activate_initial_goal(0);
    quell_initial_goal_activator_.activate_initial_goal(1);
    const goal_lineage* root0 = make_initial_goal_lineage_.make(0);
    const goal_lineage* root1 = make_initial_goal_lineage_.make(1);

    // Expand root0 into two depth-1 goals, then retire root0.
    const resolution_lineage* rl0 = lineage_pool_.make_resolution_lineage(root0, 0);
    const std::vector<const goal_lineage*> depth1 = activate_children(rl0, 2);
    ASSERT_TRUE(quell_resolver_.resolve(rl0));

    // Expand the first depth-1 goal into two depth-2 goals, then retire it.
    const resolution_lineage* rl1 = lineage_pool_.make_resolution_lineage(depth1[0], 0);
    const std::vector<const goal_lineage*> depth2 = activate_children(rl1, 2);
    ASSERT_TRUE(quell_resolver_.resolve(rl1));

    ASSERT_EQ(goal_depths_.get(depth2[0]), 2u);
    const std::vector<const goal_lineage*> frontier{root1, depth1[1], depth2[0], depth2[1]};

    EXPECT_NEAR(remaining_work_.get(), sum_work(frontier), kWorkEpsilon);
    EXPECT_NEAR(remaining_work_.get(), f(0) + f(1) + 2.0 * f(2), kWorkEpsilon);
}

TEST_F(QuellWorkConservationIntegrationTest, FailedResolveLeavesRemainingWorkUnchanged) {
    // The real resolver returns false when subgoal activation fails; quell_resolver
    // must then debit nothing, or the frontier's work drifts down on every failure.
    ON_CALL(mock_activate_subgoals, activate_subgoals_and_candidates)
        .WillByDefault(Return(false));

    quell_initial_goal_activator_.activate_initial_goal(0);
    const goal_lineage* root = make_initial_goal_lineage_.make(0);
    const resolution_lineage* rl = lineage_pool_.make_resolution_lineage(root, 0);
    const double before = remaining_work_.get();

    EXPECT_FALSE(quell_real_resolver_.resolve(rl));

    EXPECT_NEAR(remaining_work_.get(), before, kWorkEpsilon);
    EXPECT_NEAR(remaining_work_.get(), f0(), kWorkEpsilon);
    // The goal is still live, so its work value must still be readable.
    EXPECT_NEAR(goal_work_values_.get(root), f0(), kWorkEpsilon);
}

}  // namespace
