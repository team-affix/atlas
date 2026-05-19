#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/mhu_elimination_generator.hpp"
#include "../../../core/hpp/infrastructure/bind_map.hpp"
#include "../../../core/hpp/infrastructure/bind_map_factory.hpp"
#include "../../../core/hpp/infrastructure/overlay_bind_map_factory.hpp"
#include "../../../core/hpp/infrastructure/unifier_factory.hpp"
#include "../../../core/hpp/infrastructure/expr_pool.hpp"
#include "../../../core/hpp/infrastructure/lineage_pool.hpp"
#include "../../../core/hpp/utility/trail.hpp"
#include "../../../core/hpp/value_objects/unify_head.hpp"

namespace {

std::vector<const resolution_lineage*> collect_elims(
    state_machine<const resolution_lineage*>& sm) {
    std::vector<const resolution_lineage*> out;
    while (!sm.done()) {
        auto v = sm.resume();
        if (v.has_value())
            out.push_back(v.value());
    }
    return out;
}

} // namespace

class MhuEliminationGeneratorTest : public ::testing::Test {
protected:
    trail t;
    expr_pool pool{t};
    bind_map common;
    lineage_pool lp;
    mhu_elimination_generator mhu{common, pool};

    bind_map_factory bmf;
    overlay_bind_map_factory obmf;
    unifier_factory uf;

    expr goal_e{expr::var{0}};
    expr head{expr::var{1}};
    rule r{&head, {&goal_e}};
    goal_lineage* gl = nullptr;
    resolution_lineage* rl = nullptr;

    void SetUp() override {
        gl = const_cast<goal_lineage*>(lp.goal(nullptr, &goal_e));
        rl = const_cast<resolution_lineage*>(lp.resolution(gl, &r));
    }

    unify_head make_head() {
        auto local = bmf.make();
        auto overlay = obmf.make(*local, common);
        auto u = uf.make(*overlay);
        return unify_head{
            std::move(local),
            std::move(overlay),
            std::move(u)};
    }
};

TEST_F(MhuEliminationGeneratorTest, RemoveHeadAfterAddAllowsReuse) {
    std::unordered_set<uint32_t> reps{0};
    mhu.add_head(rl, make_head(), reps);
    mhu.remove_head(rl);

    std::unordered_set<uint32_t> reps2{1};
    EXPECT_NO_THROW(mhu.add_head(rl, make_head(), reps2));
}

TEST_F(MhuEliminationGeneratorTest, ConstrainDrainsWithoutEliminationsForSingleHead) {
    // No rep links: constrain only removes the head (no common_.bind loop).
    // Linking reps without a prior unify binds common to naked vars and
    // whnf can recurse forever — see bind_map collapse on self-bind.
    mhu.add_head(rl, make_head(), {});

    auto sm = mhu.constrain(rl);
    auto elims = collect_elims(sm);
    EXPECT_TRUE(elims.empty());
}
