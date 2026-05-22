#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/cdcl_elimination_generator.hpp"
#include "../../../core/hpp/utility/i_trail.hpp"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::NiceMock;
using ::testing::StrictMock;

struct MockTrail : public i_trail {
    MOCK_METHOD(void, push, (), (override));
    MOCK_METHOD(void, pop, (), (override));
    MOCK_METHOD(void, log, ((std::unique_ptr<i_backtrackable>)), (override));
};

struct CdclEliminationGeneratorUnitTest : public ::testing::Test {
protected:
    NiceMock<MockTrail> trail;
    cdcl_elimination_generator cdcl{trail};

    expr goal_expr0{expr::var{0}};
    expr goal_expr1{expr::var{1}};
    expr head0{expr::var{10}};
    expr head1{expr::var{11}};
    rule rule0{&head0, {}};
    rule rule1{&head1, {}};
    goal_lineage gl0{nullptr, &goal_expr0};
    goal_lineage gl1{nullptr, &goal_expr1};
    resolution_lineage rl0{&gl0, &rule0};
    resolution_lineage rl1{&gl1, &rule1};
};

TEST_F(CdclEliminationGeneratorUnitTest, LearnUnitAvoidanceReturnsEliminationWithoutStoring) {
    lemma l{{&rl0}};
    EXPECT_EQ(cdcl.learn(l), &rl0);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnMultiAvoidanceReturnsNull) {
    lemma l{{&rl0, &rl1}};
    EXPECT_EQ(cdcl.learn(l), nullptr);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnUnitAvoidanceDoesNotLogToTrail) {
    StrictMock<MockTrail> strict_trail;
    cdcl_elimination_generator strict_cdcl{strict_trail};

    lemma l{{&rl0}};
    EXPECT_CALL(strict_trail, log(_)).Times(0);
    EXPECT_EQ(strict_cdcl.learn(l), &rl0);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnMultiAvoidanceLogsToTrail) {
    StrictMock<MockTrail> strict_trail;
    cdcl_elimination_generator strict_cdcl{strict_trail};

    lemma l{{&rl0, &rl1}};
    EXPECT_CALL(strict_trail, log(_)).Times(AtLeast(1));
    EXPECT_EQ(strict_cdcl.learn(l), nullptr);
}
