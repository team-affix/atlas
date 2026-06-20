// Random-access rule id set: insert/erase with strict invariants; select and swap-and-pop
// index stability; copy independence.

#include <stdexcept>
#include <set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/ra_rule_id_set.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

namespace {

std::vector<rule_id> collect_rule_ids(const ra_rule_id_set& rs) {
    std::vector<rule_id> out;
    auto sm = rs.iterate();
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            out.push_back(sm.consume_yield());
    }
    return out;
}

std::set<rule_id> collect_via_select(const ra_rule_id_set& rs) {
    std::set<rule_id> out;
    for (size_t i = 0; i < rs.size(); ++i)
        out.insert(rs.select(i));
    return out;
}

}  // namespace

struct RaRuleIdSetTest : public ::testing::Test {
    ra_rule_id_set rules;
};

TEST_F(RaRuleIdSetTest, EmptyInitially) {
    EXPECT_EQ(rules.size(), 0u);
    EXPECT_THAT(collect_rule_ids(rules), IsEmpty());
}

TEST_F(RaRuleIdSetTest, InsertIncreasesSize) {
    rules.insert(0);
    rules.insert(1);
    EXPECT_EQ(rules.size(), 2u);
}

TEST_F(RaRuleIdSetTest, ContainsReturnsFalseWhenEmpty) {
    EXPECT_FALSE(rules.contains(0));
}

TEST_F(RaRuleIdSetTest, ContainsReturnsTrueAfterInsert) {
    rules.insert(0);
    rules.insert(1);
    EXPECT_TRUE(rules.contains(0));
    EXPECT_TRUE(rules.contains(1));
    EXPECT_FALSE(rules.contains(2));
}

TEST_F(RaRuleIdSetTest, ContainsReturnsFalseAfterErase) {
    rules.insert(0);
    rules.erase(0);
    EXPECT_FALSE(rules.contains(0));
}

TEST_F(RaRuleIdSetTest, DuplicateInsertThrows) {
    rules.insert(0);
    EXPECT_THROW(rules.insert(0), std::logic_error);
}

TEST_F(RaRuleIdSetTest, EraseRemovesRuleId) {
    rules.insert(0);
    rules.erase(0);
    EXPECT_EQ(rules.size(), 0u);
}

TEST_F(RaRuleIdSetTest, EraseOnMissingThrows) {
    EXPECT_THROW(rules.erase(0), std::logic_error);
}

TEST_F(RaRuleIdSetTest, EraseTwiceThrows) {
    rules.insert(0);
    rules.erase(0);
    EXPECT_THROW(rules.erase(0), std::logic_error);
}

TEST_F(RaRuleIdSetTest, IterateYieldsAllRuleIds) {
    rules.insert(0);
    rules.insert(1);
    EXPECT_THAT(collect_rule_ids(rules), UnorderedElementsAre(0, 1));
}

TEST_F(RaRuleIdSetTest, SelectYieldsAllRuleIds) {
    rules.insert(0);
    rules.insert(1);
    EXPECT_THAT(collect_via_select(rules), UnorderedElementsAre(0, 1));
}

TEST_F(RaRuleIdSetTest, SwapAndPopKeepsRemainingRulesSelectable) {
    rules.insert(0);
    rules.insert(1);
    rules.insert(2);
    rules.erase(1);
    EXPECT_EQ(rules.size(), 2u);
    EXPECT_THAT(collect_via_select(rules), UnorderedElementsAre(0, 2));
}

TEST_F(RaRuleIdSetTest, CopyPreservesRuleIds) {
    rules.insert(0);
    rules.insert(1);
    auto snapshot = rules.copy();
    EXPECT_THAT(collect_rule_ids(snapshot), UnorderedElementsAre(0, 1));
}

TEST_F(RaRuleIdSetTest, CopyIsIndependentOfSubsequentMutations) {
    rules.insert(0);
    auto snapshot = rules.copy();
    rules.erase(0);
    rules.insert(1);
    EXPECT_THAT(collect_rule_ids(snapshot), UnorderedElementsAre(0));
    EXPECT_THAT(collect_rule_ids(rules), UnorderedElementsAre(1));
}

TEST_F(RaRuleIdSetTest, FrontReturnsOnlyElement) {
    rules.insert(7);
    EXPECT_EQ(rules.front(), 7);
}

TEST_F(RaRuleIdSetTest, FrontOnEmptyThrows) {
    EXPECT_THROW(rules.front(), std::out_of_range);
}
