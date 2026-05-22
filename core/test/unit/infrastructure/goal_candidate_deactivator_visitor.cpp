#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/goal_candidate_deactivator_visitor.hpp"
#include "../../../core/hpp/interfaces/i_mhu_elimination_generator.hpp"
#include "../../../core/hpp/interfaces/i_candidate_deactivator.hpp"
#include "../../../core/hpp/interfaces/i_elimination_backlog.hpp"
#include "../../../core/hpp/interfaces/i_deactivate_candidate_translation_map.hpp"
#include "../../../core/hpp/interfaces/i_lineage_pool.hpp"

using ::testing::NiceMock;
using ::testing::Return;

struct MockMhuEliminationGenerator : public i_mhu_elimination_generator {
    MOCK_METHOD(void, add_head,
        (const resolution_lineage*, unify_head, const std::unordered_set<uint32_t>&),
        (override));
    MOCK_METHOD(void, try_remove_head, (const resolution_lineage*), (override));
    MOCK_METHOD(state_machine<const resolution_lineage*>, constrain,
        (const resolution_lineage*), (override));
};

struct MockCandidateDeactivator : public i_candidate_deactivator {
    MOCK_METHOD(void, deactivate, (const resolution_lineage*), (override));
};

struct MockDeactivateCandidateTranslationMap : public i_deactivate_candidate_translation_map {
    MOCK_METHOD(void, deactivate, (const resolution_lineage*), (override));
};

struct MockEliminationBacklog : public i_elimination_backlog {
    MOCK_METHOD(void, insert, (const resolution_lineage*), (override));
    MOCK_METHOD(bool, contains, (const resolution_lineage*), (override));
    MOCK_METHOD(void, constrain, (const resolution_lineage*), (override));
};

struct MockLineagePool : public i_lineage_pool {
    MOCK_METHOD(const goal_lineage*, goal, (const resolution_lineage*, const expr*), (override));
    MOCK_METHOD(const resolution_lineage*, resolution,
        (const goal_lineage*, const rule*), (override));
    MOCK_METHOD(void, pin_goal, (const goal_lineage*), ());
    MOCK_METHOD(void, pin_resolution, (const resolution_lineage*), ());
    MOCK_METHOD(void, trim, (), (override));
    MOCK_METHOD(const goal_lineage*, import_goal, (const goal_lineage*), ());
    MOCK_METHOD(const resolution_lineage*, import_resolution, (const resolution_lineage*), ());

    void pin(const goal_lineage* gl) override { pin_goal(gl); }
    void pin(const resolution_lineage* rl) override { pin_resolution(rl); }
    const goal_lineage* import(const goal_lineage* gl) override { return import_goal(gl); }
    const resolution_lineage* import(const resolution_lineage* rl) override {
        return import_resolution(rl);
    }
};

struct GoalCandidateDeactivatorVisitorTest : public ::testing::Test {
    expr goal_e{expr::var{0}};
    expr head{expr::var{10}};
    rule r{&head, {}};
    goal_lineage gl{nullptr, &goal_e};
    resolution_lineage rl{&gl, &r};

    NiceMock<MockMhuEliminationGenerator> mhu;
    NiceMock<MockCandidateDeactivator> cd;
    NiceMock<MockDeactivateCandidateTranslationMap> dctm;
    NiceMock<MockEliminationBacklog> eb;
    NiceMock<MockLineagePool> lp;
    goal_candidate_deactivator_visitor visitor{&gl, mhu, cd, eb, lp, dctm};
};

TEST_F(GoalCandidateDeactivatorVisitorTest, VisitTearsDownMhuTranslationAndCandidate) {
    EXPECT_CALL(lp, resolution(&gl, &r)).WillOnce(Return(&rl));
    EXPECT_CALL(mhu, try_remove_head(&rl)).Times(1);
    EXPECT_CALL(dctm, deactivate(&rl)).Times(1);
    EXPECT_CALL(cd, deactivate(&rl)).Times(1);

    visitor.visit(&r);
}
