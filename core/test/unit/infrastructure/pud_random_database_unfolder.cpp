#include <map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_random_database_unfolder.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {
using frontier_t = std::map<pud_derivation_id, std::vector<pud_body_goal_id>>;

struct MockOpenDatabase {
    MOCK_METHOD(pud_frontier_change, open_database, ());
};
struct MockUnfold {
    MOCK_METHOD(pud_frontier_change, unfold, (pud_derivation_id, pud_body_goal_id));
};
struct MockChooseUnfold {
    MOCK_METHOD(pud_unfold_choice, choose_unfold, ((const frontier_t&)));
};

using test_unfolder_t = pud_random_database_unfolder<
    NiceMock<MockOpenDatabase>, NiceMock<MockUnfold>, NiceMock<MockChooseUnfold>>;

pud_open_derivation open_derivation(uint32_t derivation, std::vector<pud_body_goal_id> goals) {
    pud_open_derivation opened{.derivation = pud_derivation_id{derivation}, .goals = {}};
    for (pud_body_goal_id goal : goals)
        opened.goals.push_back(pud_open_goal{.goal = goal, .candidate_count = 2});
    return opened;
}

pud_frontier_change opening(std::vector<pud_open_derivation> opened) {
    return pud_frontier_change{.opened = std::move(opened), .proven = {}, .refuted = {}, .made_unit = {}};
}

struct PudRandomDatabaseUnfolderTest : public ::testing::Test {
    NiceMock<MockOpenDatabase> open_database;
    NiceMock<MockUnfold> unfold;
    NiceMock<MockChooseUnfold> choose_unfold;
    test_unfolder_t unfolder{open_database, unfold, choose_unfold};

    static pud_frontier_change next(coroutine<pud_frontier_change, void>& unfolding) {
        unfolding.resume();
        EXPECT_TRUE(unfolding.has_yield());
        return unfolding.consume_yield();
    }
};

}

TEST_F(PudRandomDatabaseUnfolderTest, NothingHappensBeforeTheFirstResume) {
    EXPECT_CALL(open_database, open_database()).Times(0);
    coroutine<pud_frontier_change, void> unfolding = unfolder.unfold_database();
}

TEST_F(PudRandomDatabaseUnfolderTest, OpeningTheDatabaseIsTheFirstChange) {
    const pud_frontier_change opened = opening({open_derivation(1, {0})});
    EXPECT_CALL(open_database, open_database()).WillOnce(Return(opened));
    EXPECT_CALL(choose_unfold, choose_unfold(_)).Times(0);
    coroutine<pud_frontier_change, void> unfolding = unfolder.unfold_database();
    EXPECT_EQ(next(unfolding), opened);
}

TEST_F(PudRandomDatabaseUnfolderTest, NothingOpenEndsAfterTheOpening) {
    ON_CALL(open_database, open_database())
        .WillByDefault(Return(pud_frontier_change{.opened = {}, .proven = {pud_derivation_id{0}}, .refuted = {}, .made_unit = {}}));
    EXPECT_CALL(choose_unfold, choose_unfold(_)).Times(0);
    coroutine<pud_frontier_change, void> unfolding = unfolder.unfold_database();
    next(unfolding);
    unfolding.resume();
    EXPECT_TRUE(unfolding.done());
}

TEST_F(PudRandomDatabaseUnfolderTest, EveryOpenedDerivationIsOfferedWithItsGoalIds) {
    ON_CALL(open_database, open_database()).WillByDefault(Return(opening({open_derivation(1, {0}), open_derivation(2, {0, 1})})));
    EXPECT_CALL(choose_unfold, choose_unfold(frontier_t{{pud_derivation_id{1}, {0}}, {pud_derivation_id{2}, {0, 1}}}))
        .WillOnce(Return(pud_unfold_choice{pud_derivation_id{2}, 1}));
    coroutine<pud_frontier_change, void> unfolding = unfolder.unfold_database();
    next(unfolding);
    next(unfolding);
}

TEST_F(PudRandomDatabaseUnfolderTest, ChosenUnfoldIsUnfoldedAndItsChangeYielded) {
    ON_CALL(open_database, open_database()).WillByDefault(Return(opening({open_derivation(1, {0})})));
    ON_CALL(choose_unfold, choose_unfold(_)).WillByDefault(Return(pud_unfold_choice{pud_derivation_id{1}, 0}));
    const pud_frontier_change unfolded{.opened = {}, .proven = {pud_derivation_id{4}}, .refuted = {}, .made_unit = {}};
    InSequence in_order;
    EXPECT_CALL(choose_unfold, choose_unfold(_));
    EXPECT_CALL(unfold, unfold(pud_derivation_id{1}, 0u)).WillOnce(Return(unfolded));
    coroutine<pud_frontier_change, void> unfolding = unfolder.unfold_database();
    next(unfolding);
    EXPECT_EQ(next(unfolding), unfolded);
}

TEST_F(PudRandomDatabaseUnfolderTest, UnfoldedDerivationLeavesTheFrontier) {
    ON_CALL(open_database, open_database()).WillByDefault(Return(opening({open_derivation(1, {0})})));
    ON_CALL(choose_unfold, choose_unfold(_)).WillByDefault(Return(pud_unfold_choice{pud_derivation_id{1}, 0}));
    EXPECT_CALL(unfold, unfold(_, _)).WillOnce(Return(pud_frontier_change{}));
    coroutine<pud_frontier_change, void> unfolding = unfolder.unfold_database();
    next(unfolding);
    next(unfolding);
    unfolding.resume();
    EXPECT_TRUE(unfolding.done());
}

TEST_F(PudRandomDatabaseUnfolderTest, OpenedChildrenJoinAndRefutedDerivationsLeaveTheFrontier) {
    ON_CALL(open_database, open_database()).WillByDefault(Return(opening({open_derivation(1, {0}), open_derivation(2, {0})})));
    EXPECT_CALL(choose_unfold, choose_unfold(_)).WillOnce(Return(pud_unfold_choice{pud_derivation_id{1}, 0}));
    EXPECT_CALL(unfold, unfold(pud_derivation_id{1}, 0u))
        .WillOnce(Return(pud_frontier_change{
            .opened = {open_derivation(5, {1})}, .proven = {}, .refuted = {pud_derivation_id{2}}, .made_unit = {}}));
    coroutine<pud_frontier_change, void> unfolding = unfolder.unfold_database();
    next(unfolding);
    next(unfolding);
    EXPECT_CALL(choose_unfold, choose_unfold(frontier_t{{pud_derivation_id{5}, {1}}}))
        .WillOnce(Return(pud_unfold_choice{pud_derivation_id{5}, 1}));
    EXPECT_CALL(unfold, unfold(pud_derivation_id{5}, 1u)).WillOnce(Return(pud_frontier_change{}));
    next(unfolding);
}

TEST_F(PudRandomDatabaseUnfolderTest, ProvenDerivationsAreNeverOffered) {
    ON_CALL(open_database, open_database())
        .WillByDefault(Return(pud_frontier_change{
            .opened = {open_derivation(1, {0})}, .proven = {pud_derivation_id{2}}, .refuted = {}, .made_unit = {}}));
    EXPECT_CALL(choose_unfold, choose_unfold(frontier_t{{pud_derivation_id{1}, {0}}}))
        .WillOnce(Return(pud_unfold_choice{pud_derivation_id{1}, 0}));
    coroutine<pud_frontier_change, void> unfolding = unfolder.unfold_database();
    next(unfolding);
    next(unfolding);
}
