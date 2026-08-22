// Integration: remaining work conserved across quell initial activate + fact resolve.

#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/goal_depths.hpp"
#include "infrastructure/goal_work_function.hpp"
#include "infrastructure/goal_work_values.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/make_initial_goal_lineage.hpp"
#include "infrastructure/quell_goal_activator.hpp"
#include "infrastructure/quell_goal_deactivator.hpp"
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
    // A real resolver with the real quell_goal_deactivator in its deactivation
    // slot, so a successful resolve genuinely erases the parent's depth and work
    // value from the shared stores the way production does.
    using quell_goal_deactivator_t = quell_goal_deactivator<
        MockGoalDeactivator, goal_depths, goal_work_values>;
    using erasing_resolver_t = resolver<
        quell_goal_deactivator_t, MockActivateSubgoalsAndCandidates,
        MockDeactivateGoalCandidates, MockSetChosenGoalCandidate>;
    using quell_erasing_resolver_t = quell_resolver<
        erasing_resolver_t, goal_work_values, remaining_work>;

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
    NiceMock<MockGoalDeactivator> mock_srt_goal_deactivator;
    quell_goal_deactivator_t quell_goal_deactivator_{
        mock_srt_goal_deactivator, goal_depths_, goal_work_values_};
    erasing_resolver_t erasing_resolver_{
        quell_goal_deactivator_, mock_activate_subgoals, mock_deactivate_candidates,
        mock_set_chosen};
    quell_erasing_resolver_t quell_erasing_resolver_{
        erasing_resolver_, goal_work_values_, remaining_work_};

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
    //
    // Only what each child is given is asserted here. remaining_work is not, since
    // between activating the children and resolving the parent both are counted,
    // and nothing in production reads the register in that window --
    // BranchingProofTelescopesRemainingWorkToZero covers the settled totals.
    quell_initial_goal_activator_.activate_initial_goal(0);
    const goal_lineage* root = make_initial_goal_lineage_.make(0);
    const resolution_lineage* rl = lineage_pool_.make_resolution_lineage(root, 0);

    const std::vector<const goal_lineage*> children = activate_children(rl, 2);

    EXPECT_EQ(goal_depths_.get(children[0]), 1u);
    EXPECT_EQ(goal_depths_.get(children[1]), 1u);
    EXPECT_NEAR(goal_work_values_.get(children[0]), f(1), kWorkEpsilon);
    EXPECT_NEAR(goal_work_values_.get(children[1]), f(1), kWorkEpsilon);
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

TEST_F(QuellWorkConservationIntegrationTest, ResolveDebitsParentWorkThoughDeactivationErasesIt) {
    // quell_resolver reads the parent's work value BEFORE delegating, and that
    // order is load-bearing: the resolve deactivates the parent through
    // quell_goal_deactivator, which erases the entry, and goal_work_values::get is
    // an at(). Reading afterwards would throw. The unit tests cannot see this --
    // their mock returns the same number whenever it is asked -- so only the real
    // deactivator sharing the real store pins it.
    ON_CALL(mock_activate_subgoals, activate_subgoals_and_candidates)
        .WillByDefault(Return(true));

    quell_initial_goal_activator_.activate_initial_goal(0);
    const goal_lineage* root = make_initial_goal_lineage_.make(0);
    const resolution_lineage* rl = lineage_pool_.make_resolution_lineage(root, 0);
    ASSERT_NEAR(remaining_work_.get(), f0(), kWorkEpsilon);

    ASSERT_TRUE(quell_erasing_resolver_.resolve(rl));

    EXPECT_THROW(goal_work_values_.get(root), std::out_of_range);
    EXPECT_THROW(goal_depths_.get(root), std::out_of_range);
    EXPECT_NEAR(remaining_work_.get(), 0.0, kWorkEpsilon);
}

TEST_F(QuellWorkConservationIntegrationTest, BranchingProofTelescopesRemainingWorkToZero) {
    // f :- g, h.  g :- i.  i.  h.
    //
    // Credits and debits never cancel at a single step -- expanding f credits
    // 2*f(1) while debiting f(0) -- so the accounting only balances across the
    // whole proof. Every other end-to-end quell assertion proves a single ground
    // fact, where the goal is credited and debited at depth 0 and no child is
    // ever activated, leaving this cancellation unexercised.
    ON_CALL(mock_activate_subgoals, activate_subgoals_and_candidates)
        .WillByDefault(Return(true));

    quell_initial_goal_activator_.activate_initial_goal(0);
    const goal_lineage* f_goal = make_initial_goal_lineage_.make(0);

    const resolution_lineage* f_rl = lineage_pool_.make_resolution_lineage(f_goal, 0);
    const std::vector<const goal_lineage*> g_and_h = activate_children(f_rl, 2);
    ASSERT_TRUE(quell_erasing_resolver_.resolve(f_rl));
    EXPECT_NEAR(remaining_work_.get(), 2.0 * f(1), kWorkEpsilon);

    const resolution_lineage* g_rl = lineage_pool_.make_resolution_lineage(g_and_h[0], 0);
    const std::vector<const goal_lineage*> i_only = activate_children(g_rl, 1);
    ASSERT_EQ(goal_depths_.get(i_only[0]), 2u);
    ASSERT_TRUE(quell_erasing_resolver_.resolve(g_rl));
    EXPECT_NEAR(remaining_work_.get(), f(1) + f(2), kWorkEpsilon);

    // i and h are facts: an empty body credits nothing, so each only debits itself.
    ASSERT_TRUE(quell_erasing_resolver_.resolve(
        lineage_pool_.make_resolution_lineage(i_only[0], 0)));
    EXPECT_NEAR(remaining_work_.get(), f(1), kWorkEpsilon);

    ASSERT_TRUE(quell_erasing_resolver_.resolve(
        lineage_pool_.make_resolution_lineage(g_and_h[1], 0)));
    EXPECT_NEAR(remaining_work_.get(), 0.0, kWorkEpsilon);
}

}  // namespace
