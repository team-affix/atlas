// random_policy_choose: uniform policy_choose via injected sample_index.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstddef>
#include <vector>
#include "infrastructure/random_policy_choose.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/rule.hpp"

using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockRndGen {
    MOCK_METHOD(size_t, sample_index, (size_t), ());
};

struct ChoiceList {
    std::vector<mcts_choice> choices;

    size_t size() const { return choices.size(); }
    mcts_choice at(size_t i) const { return choices[i]; }
};

using test_random_policy_choose_t = random_policy_choose<mcts_choice, MockRndGen>;

} // namespace

struct RandomPolicyChooseTest : public ::testing::Test {
    StrictMock<MockRndGen> rnd_gen;
    test_random_policy_choose_t choose{rnd_gen};

    goal_lineage g0{nullptr, 0};
    const size_t node = 0;
};

TEST_F(RandomPolicyChooseTest, ReturnsChoiceAtSampledIndex) {
    ChoiceList list;
    list.choices = {rule_id{3}, rule_id{7}, rule_id{11}};
    EXPECT_CALL(rnd_gen, sample_index(3u)).WillOnce(Return(1u));
    const mcts_choice result = choose.policy_choose(node, list, list);
    EXPECT_EQ(std::get<rule_id>(result), 7u);
}

TEST_F(RandomPolicyChooseTest, PassesListSizeToSampleIndex) {
    ChoiceList list;
    list.choices = {&g0};
    EXPECT_CALL(rnd_gen, sample_index(1u)).WillOnce(Return(0u));
    const mcts_choice result = choose.policy_choose(node, list, list);
    EXPECT_EQ(std::get<const goal_lineage*>(result), &g0);
}

TEST_F(RandomPolicyChooseTest, ZeroIndexSelectsFirstChoice) {
    ChoiceList list;
    list.choices = {rule_id{4}, rule_id{8}};
    EXPECT_CALL(rnd_gen, sample_index(2u)).WillOnce(Return(0u));
    const mcts_choice result = choose.policy_choose(node, list, list);
    EXPECT_EQ(std::get<rule_id>(result), 4u);
}
