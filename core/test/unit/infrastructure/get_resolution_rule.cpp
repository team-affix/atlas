#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/get_resolution_rule.hpp"

using ::testing::Return;

struct MockGetRule {
    MOCK_METHOD(const rule*, get_rule, (rule_id), (const));
};

using test_get_resolution_rule_t = get_resolution_rule<MockGetRule>;

struct GetResolutionRuleTest : public ::testing::Test {
    static constexpr rule_id kRule = 2;

    MockGetRule get_rule;
    test_get_resolution_rule_t lookup{get_rule};

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, kRule};
    expr head{expr::var{0}};
    rule r{&head, {}};
};

TEST_F(GetResolutionRuleTest, DelegatesToRuleLookupByResolutionIdx) {
    EXPECT_CALL(get_rule, get_rule(kRule)).WillOnce(Return(&r));
    EXPECT_EQ(lookup.get(&rl), &r);
}
