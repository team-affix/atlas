// Rule id set container: insert/erase with strict invariants; duplicate insert and
// erase miss throw logic_error in debug builds.

#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_rule_id_set.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

namespace {

std::vector<rule_id> collect_rule_ids(const i_rule_id_set& rs) {
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

TEST_F(RuleIdSetTest, ContainsReturnsFalseWhenEmpty) {
    EXPECT_FALSE(rules.contains(0));
}

TEST_F(RuleIdSetTest, ContainsReturnsTrueAfterInsert) {
    rules.insert(0);
    rules.insert(1);
    EXPECT_TRUE(rules.contains(0));
    EXPECT_TRUE(rules.contains(1));
    EXPECT_FALSE(rules.contains(2));
}

TEST_F(RuleIdSetTest, ContainsReturnsFalseAfterErase) {
    rules.insert(0);
    rules.erase(0);
    EXPECT_FALSE(rules.contains(0));
}

TEST_F(RuleIdSetTest, DuplicateInsertThrows) {
    rules.insert(0);
    EXPECT_THROW(rules.insert(0), std::logic_error);
}

TEST_F(RuleIdSetTest, EraseRemovesRuleId) {
    rules.insert(0);
    rules.erase(0);
    EXPECT_EQ(rules.size(), 0u);
}

TEST_F(RuleIdSetTest, EraseOnMissingThrows) {
    EXPECT_THROW(rules.erase(0), std::logic_error);
}

TEST_F(RuleIdSetTest, EraseTwiceThrows) {
    rules.insert(0);
    rules.erase(0);
    EXPECT_THROW(rules.erase(0), std::logic_error);
}

TEST_F(RuleIdSetTest, IterateYieldsAllRuleIds) {
    rules.insert(0);
    rules.insert(1);
    EXPECT_THAT(collect_rule_ids(rules), UnorderedElementsAre(0, 1));
}

TEST_F(RuleIdSetTest, CopyPreservesRuleIds) {
    rules.insert(0);
    rules.insert(1);
    auto snapshot = rules.copy();
    EXPECT_THAT(collect_rule_ids(*snapshot), UnorderedElementsAre(0, 1));
}

TEST_F(RuleIdSetTest, CopyIsIndependentOfSubsequentMutations) {
    rules.insert(0);
    auto snapshot = rules.copy();
    rules.erase(0);
    rules.insert(1);
    EXPECT_THAT(collect_rule_ids(*snapshot), UnorderedElementsAre(0));
    EXPECT_THAT(collect_rule_ids(rules), UnorderedElementsAre(1));
}

TEST_F(RuleIdSetTest, FrontReturnsOnlyElement) {
    rules.insert(7);
    EXPECT_EQ(rules.front(), 7);
}

TEST_F(RuleIdSetTest, FrontOnEmptyThrows) {
    EXPECT_THROW(rules.front(), std::logic_error);
}
