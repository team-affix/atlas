#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_random_database_expander.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {
using frontier_t = std::set<pud_derivation_id>;

struct MockOpenDatabase {
    MOCK_METHOD(pud_frontier_change, open_database, ());
};
struct MockExpand {
    MOCK_METHOD(pud_frontier_change, expand, (pud_derivation_id, pud_body_goal_id));
};
struct MockChooseExpansion {
    MOCK_METHOD(pud_expansion_choice, choose_expansion, ((const frontier_t&)));
};

using test_expander_t = pud_random_database_expander<
    NiceMock<MockOpenDatabase>, NiceMock<MockExpand>, NiceMock<MockChooseExpansion>>;

pud_frontier_change opening(std::vector<pud_derivation_id> opened) {
    return pud_frontier_change{.opened = std::move(opened), .proven = {}, .refuted = {}};
}

struct PudRandomDatabaseExpanderTest : public ::testing::Test {
    NiceMock<MockOpenDatabase> open_database;
    NiceMock<MockExpand> expand;
    NiceMock<MockChooseExpansion> choose_expansion;
    test_expander_t expander{open_database, expand, choose_expansion};

    static pud_frontier_change next(coroutine<pud_frontier_change, void>& unfolding) {
        unfolding.resume();
        EXPECT_TRUE(unfolding.has_yield());
        return unfolding.consume_yield();
    }
};

}

TEST_F(PudRandomDatabaseExpanderTest, NothingHappensBeforeTheFirstResume) {
    EXPECT_CALL(open_database, open_database()).Times(0);
    coroutine<pud_frontier_change, void> unfolding = expander.expand_database();
}

TEST_F(PudRandomDatabaseExpanderTest, OpeningTheDatabaseIsTheFirstChange) {
    const pud_frontier_change opened = opening({pud_derivation_id{1}});
    EXPECT_CALL(open_database, open_database()).WillOnce(Return(opened));
    EXPECT_CALL(choose_expansion, choose_expansion(_)).Times(0);
    coroutine<pud_frontier_change, void> unfolding = expander.expand_database();
    EXPECT_EQ(next(unfolding), opened);
}

TEST_F(PudRandomDatabaseExpanderTest, NothingOpenEndsAfterTheOpening) {
    ON_CALL(open_database, open_database())
        .WillByDefault(Return(pud_frontier_change{.opened = {}, .proven = {pud_derivation_id{0}}, .refuted = {}}));
    EXPECT_CALL(choose_expansion, choose_expansion(_)).Times(0);
    coroutine<pud_frontier_change, void> unfolding = expander.expand_database();
    next(unfolding);
    unfolding.resume();
    EXPECT_TRUE(unfolding.done());
}

TEST_F(PudRandomDatabaseExpanderTest, EveryOpenedDerivationIsOffered) {
    ON_CALL(open_database, open_database()).WillByDefault(Return(opening({pud_derivation_id{1}, pud_derivation_id{2}})));
    EXPECT_CALL(choose_expansion, choose_expansion(frontier_t{pud_derivation_id{1}, pud_derivation_id{2}}))
        .WillOnce(Return(pud_expansion_choice{pud_derivation_id{2}, 1}));
    coroutine<pud_frontier_change, void> unfolding = expander.expand_database();
    next(unfolding);
    next(unfolding);
}

TEST_F(PudRandomDatabaseExpanderTest, ChosenUnfoldIsUnfoldedAndItsChangeYielded) {
    ON_CALL(open_database, open_database()).WillByDefault(Return(opening({pud_derivation_id{1}})));
    ON_CALL(choose_expansion, choose_expansion(_)).WillByDefault(Return(pud_expansion_choice{pud_derivation_id{1}, 0}));
    const pud_frontier_change unfolded{.opened = {}, .proven = {pud_derivation_id{4}}, .refuted = {}};
    InSequence in_order;
    EXPECT_CALL(choose_expansion, choose_expansion(_));
    EXPECT_CALL(expand, expand(pud_derivation_id{1}, 0u)).WillOnce(Return(unfolded));
    coroutine<pud_frontier_change, void> unfolding = expander.expand_database();
    next(unfolding);
    EXPECT_EQ(next(unfolding), unfolded);
}

TEST_F(PudRandomDatabaseExpanderTest, UnfoldedDerivationLeavesTheFrontier) {
    ON_CALL(open_database, open_database()).WillByDefault(Return(opening({pud_derivation_id{1}})));
    ON_CALL(choose_expansion, choose_expansion(_)).WillByDefault(Return(pud_expansion_choice{pud_derivation_id{1}, 0}));
    EXPECT_CALL(expand, expand(_, _)).WillOnce(Return(pud_frontier_change{}));
    coroutine<pud_frontier_change, void> unfolding = expander.expand_database();
    next(unfolding);
    next(unfolding);
    unfolding.resume();
    EXPECT_TRUE(unfolding.done());
}

TEST_F(PudRandomDatabaseExpanderTest, OpenedChildrenJoinAndRefutedDerivationsLeaveTheFrontier) {
    ON_CALL(open_database, open_database()).WillByDefault(Return(opening({pud_derivation_id{1}, pud_derivation_id{2}})));
    EXPECT_CALL(choose_expansion, choose_expansion(_)).WillOnce(Return(pud_expansion_choice{pud_derivation_id{1}, 0}));
    EXPECT_CALL(expand, expand(pud_derivation_id{1}, 0u))
        .WillOnce(Return(pud_frontier_change{
            .opened = {pud_derivation_id{5}}, .proven = {}, .refuted = {pud_derivation_id{2}}}));
    coroutine<pud_frontier_change, void> unfolding = expander.expand_database();
    next(unfolding);
    next(unfolding);
    EXPECT_CALL(choose_expansion, choose_expansion(frontier_t{pud_derivation_id{5}}))
        .WillOnce(Return(pud_expansion_choice{pud_derivation_id{5}, 1}));
    EXPECT_CALL(expand, expand(pud_derivation_id{5}, 1u)).WillOnce(Return(pud_frontier_change{}));
    next(unfolding);
}

TEST_F(PudRandomDatabaseExpanderTest, ProvenDerivationsAreNeverOffered) {
    ON_CALL(open_database, open_database())
        .WillByDefault(Return(pud_frontier_change{
            .opened = {pud_derivation_id{1}}, .proven = {pud_derivation_id{2}}, .refuted = {}}));
    EXPECT_CALL(choose_expansion, choose_expansion(frontier_t{pud_derivation_id{1}}))
        .WillOnce(Return(pud_expansion_choice{pud_derivation_id{1}, 0}));
    coroutine<pud_frontier_change, void> unfolding = expander.expand_database();
    next(unfolding);
    next(unfolding);
}
