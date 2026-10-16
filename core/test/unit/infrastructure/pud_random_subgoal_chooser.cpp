#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <immer/map.hpp>
#include "infrastructure/pud_random_subgoal_chooser.hpp"
#include "value_objects/expr.hpp"

using ::testing::_;
using ::testing::NiceMock;

namespace {

using body_goal_exprs_t = immer::map<body_goal_id, const expr*>;

struct DescentOps {
    MOCK_METHOD(const body_goal_exprs_t&, body_goal_exprs, (pud_node_id));
};

DescentOps* descent_ops = nullptr;

struct MockDescent {
    struct expr_view {
        pud_node_id node;
        const body_goal_exprs_t& body_goal_exprs() const { return descent_ops->body_goal_exprs(node); }
    };
    pud_node_id node;
    expr_view body_goal_expr() const { return {node}; }
};

struct MockGetNodeDescent {
    MOCK_METHOD(MockDescent, get, (pud_node_id));
};

const expr* expr_at(uintptr_t address) {
    return reinterpret_cast<const expr*>(address);
}

struct PudRandomSubgoalChooserTest : public ::testing::Test {
    NiceMock<DescentOps> ops;
    NiceMock<MockGetNodeDescent> descents;
    std::mt19937 rng{42};
    pud_random_subgoal_chooser<NiceMock<MockGetNodeDescent>> chooser{descents, rng};
    std::unordered_map<pud_node_id, body_goal_exprs_t> goals_of;

    void SetUp() override {
        descent_ops = &ops;
        ON_CALL(descents, get(_)).WillByDefault([](pud_node_id node) { return MockDescent{node}; });
        ON_CALL(ops, body_goal_exprs(_)).WillByDefault([&](pud_node_id node) -> const body_goal_exprs_t& {
            return goals_of.at(node);
        });
    }

    void TearDown() override { descent_ops = nullptr; }
};

}  // namespace

TEST_F(PudRandomSubgoalChooserTest, OnlyOpenGoalIsAlwaysChosen) {
    goals_of[1] = body_goal_exprs_t{}.set(6, expr_at(0x10));
    for (int draw = 0; draw < 50; ++draw)
        EXPECT_EQ(chooser.choose_subgoal(1), 6u);
}

TEST_F(PudRandomSubgoalChooserTest, ChosenGoalIsAlwaysOneOfTheLeafsOpenGoals) {
    goals_of[1] = body_goal_exprs_t{}.set(2, expr_at(0x10)).set(5, expr_at(0x20)).set(9, expr_at(0x30));
    for (int draw = 0; draw < 200; ++draw) {
        const body_goal_id chosen = chooser.choose_subgoal(1);
        EXPECT_TRUE(chosen == 2 || chosen == 5 || chosen == 9) << chosen;
    }
}

TEST_F(PudRandomSubgoalChooserTest, EveryOpenGoalIsEventuallyChosen) {
    goals_of[1] = body_goal_exprs_t{}.set(2, expr_at(0x10)).set(5, expr_at(0x20)).set(9, expr_at(0x30));
    std::set<body_goal_id> chosen;
    for (int draw = 0; draw < 200; ++draw)
        chosen.insert(chooser.choose_subgoal(1));
    EXPECT_EQ(chosen, (std::set<body_goal_id>{2, 5, 9}));
}

TEST_F(PudRandomSubgoalChooserTest, ChoicesAreRoughlyUniform) {
    goals_of[1] = body_goal_exprs_t{}.set(0, expr_at(0x10)).set(1, expr_at(0x20)).set(2, expr_at(0x30)).set(3, expr_at(0x40));
    std::map<body_goal_id, int> counts;
    for (int draw = 0; draw < 4000; ++draw)
        ++counts[chooser.choose_subgoal(1)];
    for (const auto& [goal, count] : counts)
        EXPECT_GT(count, 800) << goal;
}

TEST_F(PudRandomSubgoalChooserTest, SameSeedGivesTheSameChoices) {
    goals_of[1] = body_goal_exprs_t{}.set(2, expr_at(0x10)).set(5, expr_at(0x20)).set(9, expr_at(0x30));
    std::mt19937 other_rng{42};
    pud_random_subgoal_chooser<NiceMock<MockGetNodeDescent>> other{descents, other_rng};
    for (int draw = 0; draw < 100; ++draw)
        EXPECT_EQ(chooser.choose_subgoal(1), other.choose_subgoal(1));
}

TEST_F(PudRandomSubgoalChooserTest, ChoiceIsTheLeafsOwn) {
    goals_of[1] = body_goal_exprs_t{}.set(2, expr_at(0x10));
    goals_of[2] = body_goal_exprs_t{}.set(7, expr_at(0x20));
    EXPECT_EQ(chooser.choose_subgoal(2), 7u);
    EXPECT_EQ(chooser.choose_subgoal(1), 2u);
}

TEST_F(PudRandomSubgoalChooserTest, LeafWithNoOpenGoalsIsAPreconditionViolation) {
    goals_of[1] = body_goal_exprs_t{};
    EXPECT_THROW(chooser.choose_subgoal(1), std::logic_error);
}
