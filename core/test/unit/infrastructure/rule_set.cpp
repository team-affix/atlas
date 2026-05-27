// Rule set container: insert/erase idempotency, size, and iteration coverage. The
// set must enumerate each rule exactly once and stay consistent after erasures.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/rule_set.hpp"

using ::testing::IsEmpty;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

namespace {

std::vector<const rule*> collect_rules(const rule_set& rs) {
    std::vector<const rule*> out;
    auto sm = rs.iterate();
    while (!sm.done()) {
        if (auto r = sm.resume())
            out.push_back(*r);
    }
    return out;
}

}  // namespace

struct RuleSetTest : public ::testing::Test {
    rule_set rules;
    expr head0{expr::var{0}};
    expr head1{expr::var{1}};
    rule r0{&head0, {}};
    rule r1{&head1, {}};
};

TEST_F(RuleSetTest, EmptyInitially) {
    EXPECT_EQ(rules.size(), 0u);
    EXPECT_THAT(collect_rules(rules), IsEmpty());
}

TEST_F(RuleSetTest, InsertIncreasesSize) {
    rules.insert(&r0);
    rules.insert(&r1);
    EXPECT_EQ(rules.size(), 2u);
}

TEST_F(RuleSetTest, DuplicateInsertIsIdempotent) {
    rules.insert(&r0);
    rules.insert(&r0);
    EXPECT_EQ(rules.size(), 1u);
}

TEST_F(RuleSetTest, EraseRemovesRule) {
    rules.insert(&r0);
    rules.erase(&r0);
    EXPECT_EQ(rules.size(), 0u);
}

TEST_F(RuleSetTest, IterateYieldsAllRules) {
    rules.insert(&r0);
    rules.insert(&r1);
    EXPECT_THAT(collect_rules(rules), UnorderedElementsAre(&r0, &r1));
}
