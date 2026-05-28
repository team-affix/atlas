// Candidate activator: copies rule head, checks backlog and MHU acceptance, then
// stores translation map and links goal–candidate. Backlog or rejected head must skip
// side effects.

#include <gtest/gtest.h>
#include "locator_fixture.hpp"
#include <gmock/gmock.h>
#include "infrastructure/candidate_activator.hpp"
#include "interfaces/i_copier.hpp"
#include "interfaces/i_set_candidate_translation_map.hpp"
#include "interfaces/i_try_add_mhu_head.hpp"
#include "interfaces/i_is_backlogged_elimination.hpp"
#include "interfaces/i_get_goal_expr.hpp"
#include "interfaces/i_get_rule.hpp"
#include "interfaces/i_link_goal_candidate.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

struct MockCopier : public i_copier {
    MOCK_METHOD(const expr*, copy, (const expr*, translation_map&), (const, override));
};

struct MockSetCandidateTranslationMap : public i_set_candidate_translation_map {
    MOCK_METHOD(void, set, (const resolution_lineage*, translation_map), (override));
};

struct MockTryAddMhuHead : public i_try_add_mhu_head {
    MOCK_METHOD(bool, try_add_head,
        (const resolution_lineage*, const expr*, const expr*), (override));
};

struct MockIsBackloggedElimination : public i_is_backlogged_elimination {
    MOCK_METHOD(bool, is_backlogged_elimination, (const resolution_lineage*), (const, override));
};

struct MockGetGoalExpr : public i_get_goal_expr {
    MOCK_METHOD(const expr*, get, (const goal_lineage*), (const, override));
};

struct MockGetRule : public i_get_rule {
    MOCK_METHOD(const rule*, get, (rule_id), (const, override));
};

struct MockLinkGoalCandidate : public i_link_goal_candidate {
    MOCK_METHOD(void, link_goal_candidate, (const goal_lineage*, rule_id), (override));
};

struct CandidateActivatorTest : public ::testing::Test {
    static constexpr rule_id kRule = 0;
    static constexpr subgoal_id kGoal = 0;

    locator loc;
    MockCopier copier;
    MockSetCandidateTranslationMap set_map;
    MockTryAddMhuHead mhu;
    MockIsBackloggedElimination is_backlogged;
    MockGetGoalExpr get_goal_expr;
    MockGetRule get_rule;
    MockLinkGoalCandidate link;
    candidate_activator activator;

    CandidateActivatorTest() : activator(init_activator()) {}

    candidate_activator init_activator() {
        loc.bind_as<i_copier>(copier);
        loc.bind_as<i_set_candidate_translation_map>(set_map);
        loc.bind_as<i_try_add_mhu_head>(mhu);
        loc.bind_as<i_is_backlogged_elimination>(is_backlogged);
        loc.bind_as<i_get_goal_expr>(get_goal_expr);
        loc.bind_as<i_get_rule>(get_rule);
        loc.bind_as<i_link_goal_candidate>(link);
        return candidate_activator{loc};
    }

    expr goal_e{expr::var{0}};
    expr head{expr::var{10}};
    expr body_subgoal{expr::var{1}};
    expr copied_head{expr::var{99}};
    rule idx{&head, {&body_subgoal}};
    goal_lineage parent{nullptr, kGoal};
    resolution_lineage rl{&parent, kRule};
};

TEST_F(CandidateActivatorTest, BackloggedSkipsAllSideEffects) {
    EXPECT_CALL(is_backlogged, is_backlogged_elimination(&rl)).WillOnce(Return(true));
    EXPECT_CALL(copier, copy).Times(0);
    EXPECT_CALL(set_map, set).Times(0);
    EXPECT_CALL(link, link_goal_candidate).Times(0);
    activator.activate(&rl);
}

TEST_F(CandidateActivatorTest, RejectedHeadSkipsMapAndLink) {
    EXPECT_CALL(is_backlogged, is_backlogged_elimination(&rl)).WillOnce(Return(false));
    EXPECT_CALL(get_rule, get(kRule)).WillOnce(Return(&idx));
    EXPECT_CALL(copier, copy(&head, _)).WillOnce(Return(&copied_head));
    EXPECT_CALL(get_goal_expr, get(&parent)).WillOnce(Return(&goal_e));
    EXPECT_CALL(mhu, try_add_head(&rl, &goal_e, &copied_head)).WillOnce(Return(false));
    EXPECT_CALL(set_map, set).Times(0);
    EXPECT_CALL(link, link_goal_candidate).Times(0);
    activator.activate(&rl);
}

TEST_F(CandidateActivatorTest, AcceptedTernaryHeadPassedToMhu) {
    expr a{expr::var{1}};
    expr b{expr::var{2}};
    expr c{expr::var{3}};
    expr ternary_head{expr::functor{"h", {&a, &b, &c}}};
    expr copied_ternary{expr::functor{"h", {&a, &b, &c}}};
    rule ternary_rule{&ternary_head, {&body_subgoal}};

    EXPECT_CALL(is_backlogged, is_backlogged_elimination(&rl)).WillOnce(Return(false));
    EXPECT_CALL(get_rule, get(kRule)).WillOnce(Return(&ternary_rule));
    EXPECT_CALL(copier, copy(&ternary_head, _)).WillOnce(Return(&copied_ternary));
    EXPECT_CALL(get_goal_expr, get(&parent)).WillOnce(Return(&goal_e));
    EXPECT_CALL(mhu, try_add_head(&rl, &goal_e, &copied_ternary)).WillOnce(Return(true));
    EXPECT_CALL(set_map, set(&rl, _)).Times(1);
    EXPECT_CALL(link, link_goal_candidate(&parent, kRule)).Times(1);

    activator.activate(&rl);
}

TEST_F(CandidateActivatorTest, AcceptedHeadSetsMapAndLinks) {
    EXPECT_CALL(is_backlogged, is_backlogged_elimination(&rl)).WillOnce(Return(false));
    EXPECT_CALL(get_rule, get(kRule)).WillOnce(Return(&idx));
    EXPECT_CALL(copier, copy(&head, _)).WillOnce(Return(&copied_head));
    EXPECT_CALL(get_goal_expr, get(&parent)).WillOnce(Return(&goal_e));
    EXPECT_CALL(mhu, try_add_head(&rl, &goal_e, &copied_head)).WillOnce(Return(true));
    EXPECT_CALL(set_map, set(&rl, _)).Times(1);
    EXPECT_CALL(link, link_goal_candidate(&parent, kRule)).Times(1);
    activator.activate(&rl);
}
