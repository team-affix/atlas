// Rule set factories: make() returns the expected concrete type behind i_rule_id_set.

#include <gtest/gtest.h>
#include "infrastructure/rule_id_set_factory.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "infrastructure/ra_rule_id_set.hpp"

TEST(RuleIdSetFactoryTest, CreateReturnsRuleIdSet) {
    rule_id_set_factory factory;
    auto set = factory.make();
    EXPECT_NE(dynamic_cast<rule_id_set*>(set.get()), nullptr);
    EXPECT_EQ(dynamic_cast<ra_rule_id_set*>(set.get()), nullptr);
}

TEST(RaRuleIdSetFactoryTest, CreateReturnsRaRuleIdSet) {
    ra_rule_id_set_factory factory;
    auto set = factory.make();
    EXPECT_NE(dynamic_cast<ra_rule_id_set*>(set.get()), nullptr);
    EXPECT_EQ(dynamic_cast<rule_id_set*>(set.get()), nullptr);
}
