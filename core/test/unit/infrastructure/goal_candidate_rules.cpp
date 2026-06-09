// Goal–candidate rule id index: insert registers an empty bucket per goal via factory;
// get/link/unlink require a registered goal (.at → out_of_range); duplicate insert/erase
// assert (logic_error).

#include <memory>
#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_candidate_rule_id_set_factory.hpp"

using ::testing::ByMove;
using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

namespace {

std::vector<rule_id> collect_rule_ids(i_rule_id_set& rs) {
    std::vector<rule_id> out;
    auto sm = rs.iterate();
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            out.push_back(sm.consume_yield());
    }
    return out;
}

}  // namespace

struct MockCandidateRuleSetFactory : public i_candidate_rule_id_set_factory {
    MOCK_METHOD(std::unique_ptr<i_rule_id_set>, make, (), (const, override));
};

struct GoalCandidateRulesTest : public ::testing::Test {
    static constexpr rule_id kRule0 = 0;
    static constexpr rule_id kRule1 = 1;

    testing::NiceMock<MockCandidateRuleSetFactory> factory;
    goal_candidate_rules index{factory};
    goal_lineage gl{nullptr, 0};
    resolution_lineage rl{&gl, kRule0};

    void SetUp() override {
        ON_CALL(factory, make()).WillByDefault([] {
            return std::make_unique<rule_id_set>();
        });
    }
};

TEST_F(GoalCandidateRulesTest, GetOnUnknownGoalThrows) {
    EXPECT_THROW(index.get(&gl), std::out_of_range);
}

TEST_F(GoalCandidateRulesTest, ConstGetOnUnknownGoalThrows) {
    const goal_candidate_rules& cref = index;
    EXPECT_THROW(cref.get(&gl), std::out_of_range);
}

TEST_F(GoalCandidateRulesTest, InsertInitializesEmptySet) {
    EXPECT_CALL(factory, make()).WillOnce(Return(ByMove(std::make_unique<rule_id_set>())));
    index.insert(&gl);
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), IsEmpty());
}

TEST_F(GoalCandidateRulesTest, DuplicateInsertThrows) {
    EXPECT_CALL(factory, make()).WillOnce(Return(ByMove(std::make_unique<rule_id_set>())));
    index.insert(&gl);
    EXPECT_THROW(index.insert(&gl), std::logic_error);
}

TEST_F(GoalCandidateRulesTest, LinkAddsRuleToGoal) {
    EXPECT_CALL(factory, make()).WillOnce(Return(ByMove(std::make_unique<rule_id_set>())));
    index.insert(&gl);
    index.link_goal_candidate(&gl, kRule0);
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), UnorderedElementsAre(kRule0));
}

TEST_F(GoalCandidateRulesTest, LinkOnUnknownGoalThrows) {
    EXPECT_THROW(index.link_goal_candidate(&gl, kRule0), std::out_of_range);
}

TEST_F(GoalCandidateRulesTest, LinkDuplicateRuleThrows) {
    EXPECT_CALL(factory, make()).WillOnce(Return(ByMove(std::make_unique<rule_id_set>())));
    index.insert(&gl);
    index.link_goal_candidate(&gl, kRule0);
    EXPECT_THROW(index.link_goal_candidate(&gl, kRule0), std::logic_error);
}

TEST_F(GoalCandidateRulesTest, UnlinkRemovesRule) {
    EXPECT_CALL(factory, make()).WillOnce(Return(ByMove(std::make_unique<rule_id_set>())));
    index.insert(&gl);
    index.link_goal_candidate(&gl, kRule0);
    index.link_goal_candidate(&gl, kRule1);
    index.unlink_goal_candidate(&gl, kRule0);
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), UnorderedElementsAre(kRule1));
}

TEST_F(GoalCandidateRulesTest, UnlinkOnUnknownGoalThrows) {
    EXPECT_THROW(index.unlink_goal_candidate(&gl, kRule0), std::out_of_range);
}

TEST_F(GoalCandidateRulesTest, UnlinkMissingRuleThrows) {
    EXPECT_CALL(factory, make()).WillOnce(Return(ByMove(std::make_unique<rule_id_set>())));
    index.insert(&gl);
    index.link_goal_candidate(&gl, kRule0);
    EXPECT_THROW(index.unlink_goal_candidate(&gl, kRule1), std::logic_error);
}

TEST_F(GoalCandidateRulesTest, EraseRemovesGoalBucket) {
    EXPECT_CALL(factory, make()).WillOnce(Return(ByMove(std::make_unique<rule_id_set>())));
    index.insert(&gl);
    index.link_goal_candidate(&gl, kRule0);
    index.erase(&gl);
    EXPECT_THROW(index.get(&gl), std::out_of_range);
}

TEST_F(GoalCandidateRulesTest, EraseOnUnknownGoalThrows) {
    EXPECT_THROW(index.erase(&gl), std::logic_error);
}

TEST_F(GoalCandidateRulesTest, EraseTwiceThrows) {
    EXPECT_CALL(factory, make()).WillOnce(Return(ByMove(std::make_unique<rule_id_set>())));
    index.insert(&gl);
    index.erase(&gl);
    EXPECT_THROW(index.erase(&gl), std::logic_error);
}

TEST_F(GoalCandidateRulesTest, ClearGoalCandidateRuleIdsEmptiesAllGoals) {
    goal_lineage gl_other{nullptr, 1};
    EXPECT_CALL(factory, make())
        .WillOnce(Return(ByMove(std::make_unique<rule_id_set>())))
        .WillOnce(Return(ByMove(std::make_unique<rule_id_set>())));
    index.insert(&gl);
    index.insert(&gl_other);
    index.link_goal_candidate(&gl, kRule0);
    index.link_goal_candidate(&gl_other, kRule1);
    index.clear_goal_candidate_rule_ids();
    EXPECT_THROW(index.get(&gl), std::out_of_range);
    EXPECT_THROW(index.get(&gl_other), std::out_of_range);
}
