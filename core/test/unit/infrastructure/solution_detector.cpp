#include <gtest/gtest.h>
#include <unordered_set>
#include "../../../core/hpp/infrastructure/solution_detector.hpp"
#include "../../../core/hpp/interfaces/i_active_goals.hpp"

namespace {

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

} // namespace

TEST(SolutionDetectorTest, EmptyActiveGoalsMeansSolution) {
    fake_active_goals ag;
    solution_detector detector{ag};
    EXPECT_TRUE(detector.detect());
}

TEST(SolutionDetectorTest, NonemptyActiveGoalsIsNotSolution) {
    fake_active_goals ag;
    goal_lineage gl{nullptr, nullptr};
    ag.insert(&gl);
    solution_detector detector{ag};
    EXPECT_FALSE(detector.detect());
}
