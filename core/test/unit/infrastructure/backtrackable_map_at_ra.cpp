// backtrackable_map_at_ra_insert / _erase mutate the inner set at M::at(key),
// re-looking up the key on backtrack so the undo survives outer-node churn.
// Unit tests use a plain map-of-set (whose inner insert/erase also work when
// their return values are ignored, matching ra_rule_id_set semantics).

#include <gtest/gtest.h>
#include <set>
#include <unordered_map>
#include "infrastructure/backtrackable_map_at_ra_insert.hpp"
#include "infrastructure/backtrackable_map_at_ra_erase.hpp"

using map_of_set = std::unordered_map<int, std::set<int>>;

struct BacktrackableMapAtRaTest : public ::testing::Test {
protected:
    map_of_set m;
    void SetUp() override { m.emplace(1, std::set<int>{}); }
};

TEST_F(BacktrackableMapAtRaTest, InsertAddsToInnerSet) {
    backtrackable_map_at_ra_insert<map_of_set> op{1, 42};
    op.capture(m);
    op.invoke();
    EXPECT_EQ(m.at(1).count(42), 1u);
}

TEST_F(BacktrackableMapAtRaTest, InsertBacktrackRemovesFromInnerSet) {
    backtrackable_map_at_ra_insert<map_of_set> op{1, 42};
    op.capture(m);
    op.invoke();
    op.backtrack();
    EXPECT_EQ(m.at(1).count(42), 0u);
}

TEST_F(BacktrackableMapAtRaTest, EraseRemovesFromInnerSet) {
    m.at(1).insert(42);
    backtrackable_map_at_ra_erase<map_of_set> op{1, 42};
    op.capture(m);
    op.invoke();
    EXPECT_EQ(m.at(1).count(42), 0u);
}

TEST_F(BacktrackableMapAtRaTest, EraseBacktrackReinsertsIntoInnerSet) {
    m.at(1).insert(42);
    backtrackable_map_at_ra_erase<map_of_set> op{1, 42};
    op.capture(m);
    op.invoke();
    op.backtrack();
    EXPECT_EQ(m.at(1).count(42), 1u);
}

TEST_F(BacktrackableMapAtRaTest, BacktrackTargetsReinsertedOuterNode) {
    backtrackable_map_at_ra_insert<map_of_set> op{1, 42};
    op.capture(m);
    op.invoke();
    // Simulate the outer node being erased and re-inserted at a new address.
    m.erase(1);
    m.emplace(1, std::set<int>{42});
    op.backtrack();
    EXPECT_EQ(m.at(1).count(42), 0u);
}
