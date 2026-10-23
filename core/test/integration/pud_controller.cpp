#include <deque>
#include <set>
#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "pud_frontier_test_fixture.hpp"

using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::IsEmpty;

// The controller over several modules: one frontier across all of them, with an expansion in one
// module reaching the queries of every module that depends on it. Registering modules whose data
// was already worked on puts the same frontier back without re-deriving any refutation.
struct PudControllerIntegrationTest : public ::testing::Test {
    pud_frontier_test_fixture h;
    std::set<pud_global_node_id> live;
    std::deque<pud_data> reloaded;
    pud_controller<pud_data, expr_pool> reloaded_controller{h.exprs};

    void track(const pud_frontier_change& change) {
        live.insert(change.offspring.begin(), change.offspring.end());
        for (pud_global_node_id refuted : change.refuted)
            live.erase(refuted);
    }

    pud_frontier_change register_module() {
        const pud_frontier_change change = h.register_module();
        track(change);
        return change;
    }

    pud_frontier_change expand(pud_global_node_id leaf, pud_body_goal_id goal) {
        const pud_frontier_change change = h.expand(leaf, goal);
        live.erase(leaf);
        track(change);
        return change;
    }

    std::set<pud_global_node_id> reload_everything(std::vector<pud_global_node_id>& refuted_on_reload) {
        std::set<pud_global_node_id> reloaded_frontier;
        for (const pud_data& data : h.registered) {
            const pud_frontier_change change = reloaded_controller.register_module(reloaded.emplace_back(data));
            reloaded_frontier.insert(change.offspring.begin(), change.offspring.end());
            refuted_on_reload.insert(refuted_on_reload.end(), change.refuted.begin(), change.refuted.end());
        }
        return reloaded_frontier;
    }

    const pud_data& reloaded_data_of(pud_module_id module) const {
        for (const pud_data& data : reloaded) {
            if (data.get_module_id() == module)
                return data;
        }
        throw std::out_of_range("module not reloaded");
    }
};

TEST_F(PudControllerIntegrationTest, RegisteringAModuleBeforeItsDependencyFails) {
    h.depend_on(99);
    h.axiom(h.atom("q"), {}, 0);
    EXPECT_THROW(h.register_module(), std::logic_error);
}

TEST_F(PudControllerIntegrationTest, NodesWithTheSameLocalIdInTwoModulesStayApart) {
    const pud_global_node_id fact = h.axiom(h.functor("p", {h.atom("a")}), {}, 0);
    h.register_module();
    h.depend_on(fact.module);
    const pud_global_node_id query = h.axiom(h.functor("q", {h.var(0)}), {h.functor("p", {h.var(0)})}, 1);
    h.register_module();
    ASSERT_EQ(fact.node, query.node);
    const pud_frontier_change change = h.expand(query, 0);
    EXPECT_EQ(h.children_of(query), change.offspring);
    EXPECT_THAT(h.children_of(fact), IsEmpty());
    EXPECT_TRUE(h.data_of(fact.module).check_leaf(fact.node));
    EXPECT_FALSE(h.refuted(fact));
}

TEST_F(PudControllerIntegrationTest, ExpandingADependencysLeafKeepsADependentsQueryResolvable) {
    h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
    const pud_global_node_id step =
        h.axiom(h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
    h.register_module();
    h.depend_on(step.module);
    const pud_global_node_id query =
        h.axiom(h.atom("q"), {h.functor("nat", {h.functor("s", {h.functor("s", {h.atom("z")})})})}, 0);
    h.register_module();
    h.expand(step, 0);
    EXPECT_FALSE(h.refuted(query));
    const pud_frontier_change change = h.expand(query, 0);
    ASSERT_EQ(change.offspring.size(), 1u);
    EXPECT_EQ(change.offspring.front().module, query.module);
    EXPECT_EQ(h.expand(change.offspring.front(), 1).offspring.size(), 1u);
}

TEST_F(PudControllerIntegrationTest, ARefutationInADependencyRefutesTheDependentsLeavesThatNeededIt) {
    h.axiom(h.functor("base", {h.atom("c")}), {}, 0);
    h.axiom(h.functor("check", {h.atom("a")}), {}, 0);
    const pud_global_node_id mid =
        h.axiom(h.functor("mid", {h.var(0)}), {h.functor("base", {h.var(0)}), h.functor("check", {h.var(0)})}, 1);
    h.register_module();
    h.depend_on(mid.module);
    const pud_global_node_id top = h.axiom(h.atom("top"), {h.functor("mid", {h.atom("c")})}, 0);
    const pud_global_node_id other = h.axiom(h.atom("other"), {h.functor("check", {h.atom("a")})}, 0);
    h.register_module();
    const pud_frontier_change change = h.expand(mid, 0);
    EXPECT_THAT(change.refuted, Contains(top));
    EXPECT_TRUE(h.refuted(mid));
    EXPECT_TRUE(h.refuted(top));
    EXPECT_FALSE(h.refuted(other));
}

TEST_F(PudControllerIntegrationTest, ReloadedModulesPutBackTheSameFrontierWithoutRederivingRefutations) {
    h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
    const pud_global_node_id step =
        h.axiom(h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
    h.axiom(h.functor("color", {h.atom("red")}), {}, 0);
    h.axiom(h.functor("color", {h.atom("blue")}), {}, 0);
    h.axiom(h.functor("warm", {h.atom("red")}), {}, 0);
    register_module();
    h.depend_on(step.module);
    const pud_global_node_id warm_color =
        h.axiom(h.functor("q", {h.var(0)}), {h.functor("color", {h.var(0)}), h.functor("warm", {h.var(0)})}, 1);
    const pud_global_node_id green = h.axiom(h.atom("g"), {h.functor("color", {h.atom("green")})}, 0);
    const pud_global_node_id number = h.axiom(h.functor("n", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    EXPECT_THAT(register_module().refuted, ElementsAre(green));
    expand(step, 0);
    const pud_frontier_change colors = expand(warm_color, 0);
    ASSERT_EQ(colors.offspring.size(), 1u);
    ASSERT_EQ(colors.refuted.size(), 1u);
    const pud_frontier_change numbers = expand(number, 0);
    ASSERT_EQ(numbers.offspring.size(), 2u);

    std::vector<pud_global_node_id> refuted_on_reload;
    EXPECT_EQ(reload_everything(refuted_on_reload), live);
    EXPECT_THAT(refuted_on_reload, IsEmpty());

    const pud_global_node_id red = colors.offspring.front();
    const pud_frontier_change original = h.expand(red, 1);
    EXPECT_EQ(reloaded_controller.expand(red, 1), original);
    for (pud_node_id child : h.data_of(red.module).get_children(red.node))
        EXPECT_EQ(reloaded_data_of(red.module).get_inference(child), h.data_of(red.module).get_inference(child));
}

TEST_F(PudControllerIntegrationTest, AReloadedTreeKeepsExpandingTheWayTheOriginalDoes) {
    h.axiom(h.functor("nat", {h.atom("z")}), {}, 0);
    const pud_global_node_id step =
        h.axiom(h.functor("nat", {h.functor("s", {h.var(0)})}), {h.functor("nat", {h.var(0)})}, 1);
    register_module();
    h.depend_on(step.module);
    const pud_global_node_id number = h.axiom(h.functor("n", {h.var(0)}), {h.functor("nat", {h.var(0)})}, 1);
    register_module();
    pud_global_node_id deepest = number;
    pud_body_goal_id goal = 0;
    for (int unfold = 0; unfold < 4; ++unfold) {
        const pud_frontier_change change = expand(deepest, goal);
        ASSERT_EQ(change.offspring.size(), 2u) << "unfold " << unfold;
        deepest = change.offspring.back();
        ++goal;
    }
    std::vector<pud_global_node_id> refuted_on_reload;
    EXPECT_EQ(reload_everything(refuted_on_reload), live);
    const pud_frontier_change original = h.expand(deepest, goal);
    EXPECT_EQ(reloaded_controller.expand(deepest, goal), original);
}

TEST_F(PudControllerIntegrationTest, AReloadedRefutedNodeIsNeverActivatedAgain) {
    h.axiom(h.functor("color", {h.atom("red")}), {}, 0);
    const pud_global_node_id doomed = h.axiom(h.atom("d"), {h.functor("color", {h.atom("green")})}, 0);
    const pud_global_node_id fine = h.axiom(h.atom("f"), {h.functor("color", {h.atom("red")})}, 0);
    EXPECT_THAT(register_module().refuted, ElementsAre(doomed));
    std::vector<pud_global_node_id> refuted_on_reload;
    const std::set<pud_global_node_id> reloaded_frontier = reload_everything(refuted_on_reload);
    EXPECT_FALSE(reloaded_frontier.contains(doomed));
    EXPECT_TRUE(reloaded_frontier.contains(fine));
    EXPECT_THAT(refuted_on_reload, IsEmpty());
}
