// Rule id set container: insert/erase idempotency, size, and iteration coverage.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/rule_id_set.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

namespace {

std::vector<rule_id> collect_rule_ids(const rule_id_set& rs) {
    std::vector<rule_id> out;
    auto sm = rs.iterate();
    while (!sm.done()) {
        if (auto r = sm.resume())
            out.push_back(*r);
    }
    return out;
}

}  // namespace

struct RuleIdSetTest : public ::testing::Test {
    rule_id_set rules;
};

TEST_F(RuleIdSetTest, EmptyInitially) {
    EXPECT_EQ(rules.size(), 0u);
    EXPECT_THAT(collect_rule_ids(rules), IsEmpty());
}

TEST_F(RuleIdSetTest, InsertIncreasesSize) {
    rules.insert(0);
    rules.insert(1);
    EXPECT_EQ(rules.size(), 2u);
}

TEST_F(RuleIdSetTest, DuplicateInsertIsIdempotent) {
    rules.insert(0);
    rules.insert(0);
    EXPECT_EQ(rules.size(), 1u);
}

TEST_F(RuleIdSetTest, EraseRemovesRuleId) {
    rules.insert(0);
    rules.erase(0);
    EXPECT_EQ(rules.size(), 0u);
}

TEST_F(RuleIdSetTest, IterateYieldsAllRuleIds) {
    rules.insert(0);
    rules.insert(1);
    EXPECT_THAT(collect_rule_ids(rules), UnorderedElementsAre(0, 1));
}
