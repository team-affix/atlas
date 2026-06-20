// Rule set factories: make() returns the expected concrete type by value.

#include <gtest/gtest.h>
#include "infrastructure/rule_id_set_factory.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "infrastructure/ra_rule_id_set.hpp"

TEST(RuleIdSetFactoryTest, CreateReturnsEmptyRuleIdSet) {
    rule_id_set_factory factory;
    rule_id_set set = factory.make();
    EXPECT_EQ(set.size(), 0u);
}

TEST(RaRuleIdSetFactoryTest, CreateReturnsEmptyRaRuleIdSet) {
    ra_rule_id_set_factory factory;
    ra_rule_id_set set = factory.make();
    EXPECT_EQ(set.size(), 0u);
}
