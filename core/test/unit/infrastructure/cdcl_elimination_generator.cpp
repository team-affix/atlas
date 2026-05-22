#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/cdcl_elimination_generator.hpp"
#include "../../../core/hpp/utility/i_trail.hpp"

using ::testing::_;
using ::testing::NiceMock;

struct MockTrail : public i_trail {
    MOCK_METHOD(void, push, (), (override));
    MOCK_METHOD(void, pop, (), (override));
    MOCK_METHOD(void, log, ((std::unique_ptr<i_backtrackable>)), (override));
};

struct CdclEliminationGeneratorUnitTest : public ::testing::Test {
protected:
    NiceMock<MockTrail> trail;
    cdcl_elimination_generator cdcl{trail};

    expr goal_expr{expr::var{0}};
    expr head0{expr::var{10}};
    expr head1{expr::var{11}};
    rule rule0{&head0, {}};
    rule rule1{&head1, {}};
    goal_lineage gl{nullptr, &goal_expr};
    resolution_lineage rl0{&gl, &rule0};
    resolution_lineage rl1{&gl, &rule1};
};

TEST_F(CdclEliminationGeneratorUnitTest, LearnUnitAvoidanceReturnsEliminationWithoutStoring) {
    lemma l{{&rl0}};
    EXPECT_EQ(cdcl.learn(l), &rl0);
}

TEST_F(CdclEliminationGeneratorUnitTest, LearnMultiAvoidanceReturnsNull) {
    lemma l{{&rl0, &rl1}};
    EXPECT_EQ(cdcl.learn(l), nullptr);
}
