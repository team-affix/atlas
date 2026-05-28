#include <gtest/gtest.h>
#include "locator_fixture.hpp"
#include <gmock/gmock.h>
#include "infrastructure/get_resolution_rule.hpp"
#include "interfaces/i_get_rule.hpp"

using ::testing::Return;

struct MockGetRule : public i_get_rule {
    MOCK_METHOD(const rule*, get, (rule_id), (const, override));
};

struct GetResolutionRuleTest : public ::testing::Test {
    static constexpr rule_id kRule = 2;

    locator loc;
    MockGetRule get_rule;
    get_resolution_rule lookup;

    GetResolutionRuleTest()
        : lookup(bind_and_make<get_resolution_rule, i_get_rule>(loc, get_rule)) {}
    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, kRule};
    expr head{expr::var{0}};
    rule r{&head, {}};
};

TEST_F(GetResolutionRuleTest, DelegatesToRuleLookupByResolutionIdx) {
    EXPECT_CALL(get_rule, get(kRule)).WillOnce(Return(&r));
    EXPECT_EQ(lookup.get(&rl), &r);
}
