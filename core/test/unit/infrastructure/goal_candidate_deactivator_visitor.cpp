#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/goal_candidate_deactivator_visitor.hpp"
#include "../../../core/hpp/infrastructure/lineage_pool.hpp"
#include "../../../core/hpp/interfaces/i_mhu_elimination_generator.hpp"
#include "../../../core/hpp/interfaces/i_candidate_deactivator.hpp"
#include "../../../core/hpp/interfaces/i_elimination_backlog.hpp"
#include "../../../core/hpp/interfaces/i_deactivate_candidate_translation_map.hpp"

namespace {

state_machine<const resolution_lineage*> make_empty_constrain() {
    co_return;
}

class recording_mhu : public i_mhu_elimination_generator {
public:
    std::unordered_set<const resolution_lineage*> removed;

    void add_head(const resolution_lineage*, unify_head,
        const std::unordered_set<uint32_t>&) override {}
    void try_remove_head(const resolution_lineage* rl) override { removed.insert(rl); }
    state_machine<const resolution_lineage*> constrain(const resolution_lineage*) override {
        return make_empty_constrain();
    }
};

class recording_candidate_deactivator : public i_candidate_deactivator {
public:
    std::unordered_set<const resolution_lineage*> deactivated;

    void deactivate(const resolution_lineage* rl) override { deactivated.insert(rl); }
};

class recording_translation_deactivator : public i_deactivate_candidate_translation_map {
public:
    std::unordered_set<const resolution_lineage*> deactivated;

    void deactivate(const resolution_lineage* rl) override { deactivated.insert(rl); }
};

class noop_backlog : public i_elimination_backlog {
public:
    void insert(const resolution_lineage*) override {}
    bool contains(const resolution_lineage*) override { return false; }
    void constrain(const resolution_lineage*) override {}
};

} // namespace

class GoalCandidateDeactivatorVisitorTest : public ::testing::Test {
protected:
    lineage_pool lp;
    expr goal_e{expr::var{0}};
    expr head{expr::var{10}};
    rule r{&head, {}};
    goal_lineage* gl = nullptr;
    recording_mhu mhu;
    recording_candidate_deactivator cd;
    recording_translation_deactivator dctm;
    noop_backlog eb;

    void SetUp() override {
        gl = const_cast<goal_lineage*>(lp.goal(nullptr, &goal_e));
    }
};

TEST_F(GoalCandidateDeactivatorVisitorTest, VisitTearsDownMhuTranslationAndCandidate) {
    goal_candidate_deactivator_visitor visitor{gl, mhu, cd, eb, lp, dctm};
    visitor.visit(&r);

    const resolution_lineage* rl = lp.resolution(gl, &r);
    EXPECT_TRUE(mhu.removed.count(rl));
    EXPECT_TRUE(dctm.deactivated.contains(rl));
    EXPECT_TRUE(cd.deactivated.contains(rl));
}
