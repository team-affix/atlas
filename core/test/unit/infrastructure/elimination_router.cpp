#include <gtest/gtest.h>
#include <unordered_set>
#include "../../../core/hpp/infrastructure/elimination_router.hpp"
#include "../../../core/hpp/value_objects/elimination_result.hpp"
#include "../../../core/hpp/interfaces/i_deactivated_candidate_memory.hpp"
#include "../../../core/hpp/interfaces/i_active_goals.hpp"
#include "../../../core/hpp/interfaces/i_elimination_backlog.hpp"
#include "../../../core/hpp/interfaces/i_candidate_deactivator.hpp"

namespace {

class fake_deactivated_candidate_memory : public i_deactivated_candidate_memory {
public:
    void insert(const resolution_lineage* rl) override { deactivated.insert(rl); }
    void clear() override { deactivated.clear(); }
    bool contains(const resolution_lineage* rl) const override {
        return deactivated.count(rl);
    }

    std::unordered_set<const resolution_lineage*> deactivated;
};

class fake_active_goals : public i_active_goals {
public:
    void insert(const goal_lineage* gl) override { goals.insert(gl); }
    void erase(const goal_lineage* gl) override { goals.erase(gl); }
    bool contains(const goal_lineage* gl) const override { return goals.count(gl); }
    void accept(i_visitor<const goal_lineage*>&) const override {}
    size_t size() const override { return goals.size(); }
    bool empty() const override { return goals.empty(); }

    std::unordered_set<const goal_lineage*> goals;
};

class fake_elimination_backlog : public i_elimination_backlog {
public:
    void insert(const resolution_lineage* rl) override { backlog.insert(rl); }
    bool contains(const resolution_lineage* rl) override { return backlog.count(rl); }
    void constrain(const resolution_lineage*) override {}

    std::unordered_set<const resolution_lineage*> backlog;
};

class fake_candidate_deactivator : public i_candidate_deactivator {
public:
    void deactivate(const resolution_lineage* rl) override { eliminated.insert(rl); }

    std::unordered_set<const resolution_lineage*> eliminated;
};

} // namespace

class EliminationRouterTest : public ::testing::Test {
protected:
    expr goal_e{expr::var{0}};
    expr head{expr::var{1}};
    rule idx{&head, {}};
    goal_lineage parent{nullptr, &goal_e};
    resolution_lineage rl{&parent, &idx};

    fake_deactivated_candidate_memory dcm;
    fake_active_goals ag;
    fake_elimination_backlog eb;
    fake_candidate_deactivator cd;
    elimination_router router{dcm, ag, eb, cd};
};

TEST_F(EliminationRouterTest, AlreadyDeactivatedReturnsAlreadyDeactivated) {
    dcm.insert(&rl);
    EXPECT_EQ(router.route(&rl), elimination_result::already_deactivated);
    EXPECT_TRUE(eb.backlog.empty());
    EXPECT_TRUE(cd.eliminated.empty());
}

TEST_F(EliminationRouterTest, InactiveParentAddsToBacklog) {
    EXPECT_EQ(router.route(&rl), elimination_result::added_to_backlog);
    EXPECT_TRUE(eb.backlog.contains(&rl));
    EXPECT_TRUE(cd.eliminated.empty());
}

TEST_F(EliminationRouterTest, ActiveParentEliminatesCandidate) {
    ag.insert(&parent);
    EXPECT_EQ(router.route(&rl), elimination_result::eliminated);
    EXPECT_TRUE(cd.eliminated.contains(&rl));
    EXPECT_FALSE(eb.backlog.contains(&rl));
}
