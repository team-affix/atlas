#include <random>
#include <set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_random_expansion_chooser.hpp"
#include "value_objects/pud_goal_list.hpp"

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_node_id));
};

struct PudRandomExpansionChooserTest : public ::testing::Test {
    NiceMock<MockGetNodeGoals> goals;
    std::mt19937 rng{3};
    pud_random_expansion_chooser<NiceMock<MockGetNodeGoals>> chooser{goals, rng};
    expr goal{expr::functor{1, {}}};
    pud_goal_list first_goals = pud_goal_list{.goals = {}, .next_id = 4}.adding({&goal, &goal});
    pud_goal_list second_goals = pud_goal_list{.goals = {}, .next_id = 9}.adding({&goal});

    void SetUp() override {
        ON_CALL(goals, get(1)).WillByDefault(ReturnRef(first_goals));
        ON_CALL(goals, get(2)).WillByDefault(ReturnRef(second_goals));
    }
};

}

TEST_F(PudRandomExpansionChooserTest, EveryChoiceIsAnOpenGoalOfAFrontierLeaf) {
    const std::set<pud_unfold> allowed{{.leaf = 1, .goal = 4}, {.leaf = 1, .goal = 5}, {.leaf = 2, .goal = 9}};
    std::set<pud_unfold> chosen;
    for (int draw = 0; draw < 200; ++draw)
        chosen.insert(chooser.choose_expansion({1, 2}));
    EXPECT_EQ(chosen, allowed);
}
