// mcts_decision_generator: MCTS-guided goal descent + candidate pick, then make_resolution_lineage.

#include <deque>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <set>
#include <variant>
#include <vector>
#include "infrastructure/mcts_decision_generator.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "infrastructure/ra_rule_id_set.hpp"

using ::testing::Invoke;
using ::testing::ReturnRef;
using ::testing::UnorderedElementsAre;

namespace {

auto stub_make_resolution_lineage(std::set<resolution_lineage>& resolutions) {
    return [&resolutions](const goal_lineage* goal, rule_id rid) {
        auto [it, _] = resolutions.emplace(goal, rid);
        return &*it;
    };
}

std::vector<const goal_lineage*> goal_pointers(const std::vector<mcts_choice>& choices) {
    std::vector<const goal_lineage*> out;
    out.reserve(choices.size());
    for (const mcts_choice& c : choices)
        out.push_back(std::get<const goal_lineage*>(c));
    return out;
}

std::vector<rule_id> rule_pointers(const std::vector<mcts_choice>& choices) {
    std::vector<rule_id> out;
    out.reserve(choices.size());
    for (const mcts_choice& c : choices)
        out.push_back(std::get<rule_id>(c));
    return out;
}

void link_two_children(
    srt_active_goals& goals,
    const goal_lineage* parent,
    const goal_lineage* child0,
    const goal_lineage* child1) {
    goals.insert_active_goal(child0);
    goals.insert_active_goal(child1);
    goals.link_srt_goal_batch_parent(parent);
    goals.flush_srt_goal_batch();
}

struct ScriptingMctsChoose {
    std::deque<mcts_choice> responses;
    std::vector<std::vector<mcts_choice>> goal_rounds;
    std::vector<std::vector<mcts_choice>> candidate_rounds;

    mcts_choice choose(const std::vector<mcts_choice>& choices) {
        if (!choices.empty()) {
            if (std::holds_alternative<const goal_lineage*>(choices.front()))
                goal_rounds.push_back(choices);
            else
                candidate_rounds.push_back(choices);
        }
        if (responses.empty())
            return choices.front();
        mcts_choice next = responses.front();
        responses.pop_front();
        return next;
    }
};

struct MockMakeResolutionLineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id));
};

struct MockGetGoalCandidateRuleIds {
    MOCK_METHOD(ra_rule_id_set&, get, (const goal_lineage*));
};

using test_mcts_decision_generator_t = mcts_decision_generator<
    MockMakeResolutionLineage, srt_active_goals,
    ScriptingMctsChoose, MockGetGoalCandidateRuleIds>;

struct MctsDecisionGeneratorTest : public ::testing::Test {
    srt_active_goals active_goals;
    ScriptingMctsChoose mcts_choose;
    MockMakeResolutionLineage make_resolution;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    test_mcts_decision_generator_t generator{make_resolution, active_goals,
                                        mcts_choose, get_goal_candidate_rule_ids};

    std::set<resolution_lineage> resolutions;
    ra_rule_id_set candidates;

    goal_lineage parent{nullptr, 0};
    goal_lineage parent2{nullptr, 1};
    goal_lineage child0{nullptr, 2};
    goal_lineage child1{nullptr, 3};
    goal_lineage child2{nullptr, 4};
};

}  // namespace

TEST_F(MctsDecisionGeneratorTest, GenerateSingleActiveRootPicksGoalThenCandidate) {
    active_goals.insert_active_goal(&child0);
    active_goals.flush_srt_goal_batch();
    candidates.insert(7);
    mcts_choose.responses = {mcts_choice{&child0}, mcts_choice{rule_id{7}}};

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&child0)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(make_resolution, make_resolution_lineage(&child0, 7))
        .WillOnce(stub_make_resolution_lineage(resolutions));

    const resolution_lineage* result = generator.generate();
    EXPECT_EQ(*result, (resolution_lineage{&child0, 7}));
    ASSERT_EQ(mcts_choose.goal_rounds.size(), 1u);
    EXPECT_THAT(goal_pointers(mcts_choose.goal_rounds[0]), UnorderedElementsAre(&child0));
    ASSERT_EQ(mcts_choose.candidate_rounds.size(), 1u);
    EXPECT_THAT(rule_pointers(mcts_choose.candidate_rounds[0]), UnorderedElementsAre(7));
}

TEST_F(MctsDecisionGeneratorTest, GenerateOffersAllRootsToMcts) {
    active_goals.insert_active_goal(&child0);
    active_goals.flush_srt_goal_batch();
    active_goals.insert_active_goal(&child1);
    active_goals.flush_srt_goal_batch();
    candidates.insert(0);
    mcts_choose.responses = {mcts_choice{&child1}, mcts_choice{rule_id{0}}};

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&child1)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(make_resolution, make_resolution_lineage(&child1, 0))
        .WillOnce(stub_make_resolution_lineage(resolutions));

    generator.generate();
    ASSERT_EQ(mcts_choose.goal_rounds.size(), 1u);
    EXPECT_THAT(
        goal_pointers(mcts_choose.goal_rounds[0]),
        UnorderedElementsAre(&child0, &child1));
}

TEST_F(MctsDecisionGeneratorTest, GenerateWalksBranchUntilActiveLeaf) {
    active_goals.insert_active_goal(&parent);
    active_goals.flush_srt_goal_batch();
    link_two_children(active_goals, &parent, &child0, &child1);
    candidates.insert(2);
    mcts_choose.responses = {mcts_choice{&parent}, mcts_choice{&child0}, mcts_choice{rule_id{2}}};

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&child0)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(make_resolution, make_resolution_lineage(&child0, 2))
        .WillOnce(stub_make_resolution_lineage(resolutions));

    const resolution_lineage* result = generator.generate();
    EXPECT_EQ(result->parent, &child0);
    EXPECT_EQ(result->idx, 2u);
    ASSERT_EQ(mcts_choose.goal_rounds.size(), 2u);
    EXPECT_THAT(goal_pointers(mcts_choose.goal_rounds[0]), UnorderedElementsAre(&parent));
    EXPECT_THAT(goal_pointers(mcts_choose.goal_rounds[1]), UnorderedElementsAre(&child0, &child1));
    ASSERT_EQ(mcts_choose.candidate_rounds.size(), 1u);
}

TEST_F(MctsDecisionGeneratorTest, GenerateDeepTreeRequiresMultipleGoalChoices) {
    active_goals.insert_active_goal(&parent);
    active_goals.flush_srt_goal_batch();
    active_goals.insert_active_goal(&parent2);
    active_goals.insert_active_goal(&child2);
    active_goals.link_srt_goal_batch_parent(&parent);
    active_goals.flush_srt_goal_batch();
    active_goals.insert_active_goal(&child0);
    active_goals.insert_active_goal(&child1);
    active_goals.link_srt_goal_batch_parent(&parent2);
    active_goals.flush_srt_goal_batch();
    candidates.insert(9);
    mcts_choose.responses = {
        mcts_choice{&parent},
        mcts_choice{&parent2},
        mcts_choice{&child0},
        mcts_choice{rule_id{9}},
    };

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&child0)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(make_resolution, make_resolution_lineage(&child0, 9))
        .WillOnce(stub_make_resolution_lineage(resolutions));

    const resolution_lineage* result = generator.generate();
    EXPECT_EQ(result->parent, &child0);
    EXPECT_EQ(result->idx, 9u);
    ASSERT_EQ(mcts_choose.goal_rounds.size(), 3u);
    EXPECT_THAT(goal_pointers(mcts_choose.goal_rounds[0]), UnorderedElementsAre(&parent));
    EXPECT_THAT(
        goal_pointers(mcts_choose.goal_rounds[1]),
        UnorderedElementsAre(&parent2, &child2));
    EXPECT_THAT(goal_pointers(mcts_choose.goal_rounds[2]), UnorderedElementsAre(&child0, &child1));
}

TEST_F(MctsDecisionGeneratorTest, GenerateUsesMctsSelectedRootAmongIsolatedGoals) {
    active_goals.insert_active_goal(&child0);
    active_goals.flush_srt_goal_batch();
    active_goals.insert_active_goal(&child2);
    active_goals.flush_srt_goal_batch();
    candidates.insert(1);
    mcts_choose.responses = {mcts_choice{&child2}, mcts_choice{rule_id{1}}};

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&child2)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(make_resolution, make_resolution_lineage(&child2, 1))
        .WillOnce(stub_make_resolution_lineage(resolutions));

    const resolution_lineage* result = generator.generate();
    EXPECT_EQ(result->parent, &child2);
}

TEST_F(MctsDecisionGeneratorTest, GeneratePassesAllCandidatesToMcts) {
    active_goals.insert_active_goal(&child0);
    active_goals.flush_srt_goal_batch();
    for (rule_id r = 0; r < 4; ++r)
        candidates.insert(r);
    mcts_choose.responses = {mcts_choice{&child0}, mcts_choice{rule_id{3}}};

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&child0)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(make_resolution, make_resolution_lineage(&child0, 3))
        .WillOnce(stub_make_resolution_lineage(resolutions));

    generator.generate();
    ASSERT_EQ(mcts_choose.candidate_rounds.size(), 1u);
    EXPECT_THAT(rule_pointers(mcts_choose.candidate_rounds[0]), UnorderedElementsAre(0, 1, 2, 3));
}

TEST_F(MctsDecisionGeneratorTest, GenerateSelectsSiblingBranchWhenMctsChoosesIt) {
    active_goals.insert_active_goal(&parent);
    active_goals.flush_srt_goal_batch();
    link_two_children(active_goals, &parent, &child0, &child1);
    candidates.insert(5);
    mcts_choose.responses = {mcts_choice{&parent}, mcts_choice{&child1}, mcts_choice{rule_id{5}}};

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&child1)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(make_resolution, make_resolution_lineage(&child1, 5))
        .WillOnce(stub_make_resolution_lineage(resolutions));

    const resolution_lineage* result = generator.generate();
    EXPECT_EQ(result->parent, &child1);
    EXPECT_EQ(result->idx, 5u);
}

TEST_F(MctsDecisionGeneratorTest, GenerateStopsAtFirstActiveGoalWithoutDescendingFurther) {
    active_goals.insert_active_goal(&child0);
    active_goals.flush_srt_goal_batch();
    active_goals.insert_active_goal(&child1);
    active_goals.flush_srt_goal_batch();
    candidates.insert(0);
    mcts_choose.responses = {mcts_choice{&child0}, mcts_choice{rule_id{0}}};

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&child0)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(make_resolution, make_resolution_lineage(&child0, 0))
        .WillOnce(stub_make_resolution_lineage(resolutions));
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&child1)).Times(0);

    generator.generate();
    EXPECT_EQ(mcts_choose.goal_rounds.size(), 1u);
}

TEST_F(MctsDecisionGeneratorTest, GenerateWithSingleChildLinkCollapsesToActiveLeaf) {
    active_goals.insert_active_goal(&parent);
    active_goals.flush_srt_goal_batch();
    active_goals.insert_active_goal(&child0);
    active_goals.link_srt_goal_batch_parent(&parent);
    active_goals.flush_srt_goal_batch();
    candidates.insert(6);
    mcts_choose.responses = {mcts_choice{&child0}, mcts_choice{rule_id{6}}};

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&child0)).WillOnce(ReturnRef(candidates));
    EXPECT_CALL(make_resolution, make_resolution_lineage(&child0, 6))
        .WillOnce(stub_make_resolution_lineage(resolutions));

    const resolution_lineage* result = generator.generate();
    EXPECT_EQ(result->parent, &child0);
    ASSERT_EQ(mcts_choose.goal_rounds.size(), 1u);
    EXPECT_THAT(goal_pointers(mcts_choose.goal_rounds[0]), UnorderedElementsAre(&child0));
}

TEST_F(MctsDecisionGeneratorTest, GenerateSecondCallRepeatsRootIteration) {
    active_goals.insert_active_goal(&child0);
    active_goals.flush_srt_goal_batch();
    candidates.insert(0);
    mcts_choose.responses = {
        mcts_choice{&child0},
        mcts_choice{rule_id{0}},
        mcts_choice{&child0},
        mcts_choice{rule_id{0}},
    };

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&child0))
        .Times(2)
        .WillRepeatedly(ReturnRef(candidates));
    EXPECT_CALL(make_resolution, make_resolution_lineage(&child0, 0))
        .Times(2)
        .WillRepeatedly(stub_make_resolution_lineage(resolutions));

    generator.generate();
    generator.generate();
    EXPECT_EQ(mcts_choose.goal_rounds.size(), 2u);
    EXPECT_EQ(mcts_choose.candidate_rounds.size(), 2u);
}

TEST_F(MctsDecisionGeneratorTest, GeneratePicksAmongMultipleGoalsAndCandidates) {
    link_two_children(active_goals, &parent, &child0, &child1);
    candidates.insert(0);
    candidates.insert(1);
    mcts_choose.responses = {mcts_choice{&child1}, mcts_choice{rule_id{1}}};

    EXPECT_CALL(get_goal_candidate_rule_ids, get(&child1))
        .WillRepeatedly(ReturnRef(candidates));
    EXPECT_CALL(make_resolution, make_resolution_lineage(testing::_, testing::_))
        .WillOnce(Invoke([&](const goal_lineage* gl, rule_id rid) {
            EXPECT_TRUE(gl == &child0 || gl == &child1);
            EXPECT_TRUE(rid == 0 || rid == 1);
            return stub_make_resolution_lineage(resolutions)(gl, rid);
        }));

    const resolution_lineage* result = generator.generate();
    EXPECT_EQ(result->parent, &child1);
    EXPECT_EQ(result->idx, 1);
    ASSERT_GE(mcts_choose.goal_rounds.size(), 1u);
    EXPECT_THAT(goal_pointers(mcts_choose.goal_rounds[0]), UnorderedElementsAre(&child0, &child1));
    ASSERT_GE(mcts_choose.candidate_rounds.size(), 1u);
    EXPECT_THAT(rule_pointers(mcts_choose.candidate_rounds[0]), UnorderedElementsAre(0, 1));
}
