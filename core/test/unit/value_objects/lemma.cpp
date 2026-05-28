// lemma normalizes a set of resolution lineages by dropping any resolution that is an
// ancestor of another member. Tests build synthetic lineage chains and assert the stored
// set is exactly the deepest resolutions (minimal hitting set for avoidance clauses).

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_set>
#include "value_objects/lemma.hpp"

using ::testing::UnorderedElementsAre;

struct LemmaTest : public ::testing::Test {
protected:
    resolution_lineage res0_storage{nullptr, 0};
    goal_lineage goal0_storage{nullptr, 0};
    resolution_lineage res1_storage{nullptr, 0};
    goal_lineage goal1_storage{nullptr, 0};
    resolution_lineage res2_storage{nullptr, 0};

    void build_chain(resolution_lineage*& res0,
                     goal_lineage*& goal0,
                     resolution_lineage*& res1,
                     goal_lineage*& goal1,
                     resolution_lineage*& res2) {
        res0_storage  = resolution_lineage{nullptr, 0};
        goal0_storage = goal_lineage{&res0_storage, 0};
        res1_storage  = resolution_lineage{&goal0_storage, 0};
        goal1_storage = goal_lineage{&res1_storage, 0};
        res2_storage  = resolution_lineage{&goal1_storage, 0};

        res0  = &res0_storage;
        goal0 = &goal0_storage;
        res1  = &res1_storage;
        goal1 = &goal1_storage;
        res2  = &res2_storage;
    }
};

static std::unordered_set<const resolution_lineage*> as_set(
    std::initializer_list<const resolution_lineage*> xs) {
    return {xs.begin(), xs.end()};
}

TEST_F(LemmaTest, LemmaSingleResolutionUnchanged) {
    resolution_lineage res0{nullptr, 0};
    lemma l{as_set({&res0})};
    EXPECT_THAT(l.get_resolutions(), UnorderedElementsAre(&res0));
}

TEST_F(LemmaTest, LemmaDropsAncestorWhenLeafIncluded) {
    resolution_lineage *res0, *res1, *res2;
    goal_lineage *goal0, *goal1;
    build_chain(res0, goal0, res1, goal1, res2);

    lemma l{as_set({res0, res1, res2})};
    EXPECT_THAT(l.get_resolutions(), UnorderedElementsAre(res2));
}

TEST_F(LemmaTest, LemmaDropsIntermediateAncestor) {
    resolution_lineage *res0, *res1, *res2;
    goal_lineage *goal0, *goal1;
    build_chain(res0, goal0, res1, goal1, res2);

    lemma l{as_set({res1, res2})};
    EXPECT_THAT(l.get_resolutions(), UnorderedElementsAre(res2));
}

TEST_F(LemmaTest, LemmaIdempotentForLeafOnlyInput) {
    resolution_lineage *res0, *res1, *res2;
    goal_lineage *goal0, *goal1;
    build_chain(res0, goal0, res1, goal1, res2);

    lemma l{as_set({res2})};
    EXPECT_THAT(l.get_resolutions(), UnorderedElementsAre(res2));
}

TEST_F(LemmaTest, LemmaPrunesToDeepestResolutionsOnly) {
    resolution_lineage *res0, *res1, *res2;
    goal_lineage *goal0, *goal1;
    build_chain(res0, goal0, res1, goal1, res2);

    lemma l{as_set({res2, res1})};
    EXPECT_THAT(l.get_resolutions(), UnorderedElementsAre(res2));
}
