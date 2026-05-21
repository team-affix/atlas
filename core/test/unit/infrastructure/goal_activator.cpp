#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/goal_activator.hpp"
#include "../../../core/hpp/infrastructure/lineage_pool.hpp"
#include "../../../core/hpp/infrastructure/copier.hpp"
#include "../../../core/hpp/infrastructure/var_sequencer.hpp"
#include "../../../core/hpp/infrastructure/expr_pool.hpp"
#include "../../../core/hpp/utility/trail.hpp"

namespace {

class recording_activate_goal_expr : public i_activate_goal_expr {
public:
    const goal_lineage* last_gl = nullptr;
    const expr* last_expr = nullptr;

    void activate(const goal_lineage* gl, const expr* e) override {
        last_gl = gl;
        last_expr = e;
    }
};

class recording_translation_maps : public i_get_candidate_translation_map {
public:
    translation_map& get(const resolution_lineage* rl) override {
        return maps[rl];
    }

    std::unordered_map<const resolution_lineage*, translation_map> maps;
};

} // namespace

class GoalActivatorTest : public ::testing::Test {
protected:
    trail t;
    expr_pool pool{t};
    var_sequencer vs{t};
    copier cp{vs, pool};
    lineage_pool lp;
    recording_activate_goal_expr age;
    recording_translation_maps gctm;

    expr parent_goal{expr::var{0}};
    expr child_goal{expr::var{1}};
    expr rule_head{expr::var{10}};
    rule parent_rule{&rule_head, {&child_goal}};
    resolution_lineage* res = nullptr;
    goal_lineage* parent_gl = nullptr;
    goal_lineage* child_gl = nullptr;

    void SetUp() override {
        res = const_cast<resolution_lineage*>(lp.resolution(nullptr, &parent_rule));
        parent_gl = const_cast<goal_lineage*>(lp.goal(res, &parent_goal));
        child_gl = const_cast<goal_lineage*>(lp.goal(res, &child_goal));
        gctm.maps[res] = translation_map{{1, 2}};
    }

    goal_activator activator{age, gctm, cp};
};

TEST_F(GoalActivatorTest, ActivatePassesCopiedGoalExprToGoalExprActivator) {
    activator.activate(child_gl);

    EXPECT_EQ(age.last_gl, child_gl);
    ASSERT_NE(age.last_expr, nullptr);
    EXPECT_NE(age.last_expr, child_gl->idx);
}
