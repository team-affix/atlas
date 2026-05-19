#include <gtest/gtest.h>
#include <unordered_set>
#include "../../../core/hpp/value_objects/lemma.hpp"
#include "../../../core/hpp/infrastructure/lineage_pool.hpp"

class LemmaTest : public ::testing::Test {
protected:
    lineage_pool pool;
    expr goal_expr0{expr::var{0}};
    expr rule_head0{expr::var{10}};
    expr rule_head1{expr::var{11}};
    rule rule_idx0{&rule_head0, {}};
    rule rule_idx1{&rule_head1, {}};

    // res0 (root) → goal0 → res1 → goal1 → res2
    void build_chain(const resolution_lineage*& res0,
                     const goal_lineage*& goal0,
                     const resolution_lineage*& res1,
                     const goal_lineage*& goal1,
                     const resolution_lineage*& res2) {
        res0  = pool.resolution(nullptr, &rule_idx0);
        goal0 = pool.goal(res0, &goal_expr0);
        res1  = pool.resolution(goal0, &rule_idx0);
        goal1 = pool.goal(res1, &goal_expr0);
        res2  = pool.resolution(goal1, &rule_idx0);
    }
};

static std::unordered_set<const resolution_lineage*> as_set(
    std::initializer_list<const resolution_lineage*> xs) {
    return {xs.begin(), xs.end()};
}

// ---------------------------------------------------------------------------
// Leaf-only pruning
// ---------------------------------------------------------------------------

TEST_F(LemmaTest, LemmaSingleResolutionUnchanged) {
    const resolution_lineage* res0 = pool.resolution(nullptr, &rule_idx0);
    lemma l{as_set({res0})};
    EXPECT_EQ(l.get_resolutions(), as_set({res0}));
}

TEST_F(LemmaTest, LemmaDropsAncestorWhenLeafIncluded) {
    const resolution_lineage *res0, *res1, *res2;
    const goal_lineage *goal0, *goal1;
    build_chain(res0, goal0, res1, goal1, res2);

    lemma l{as_set({res0, res1, res2})};
    EXPECT_EQ(l.get_resolutions(), as_set({res2}));
}

TEST_F(LemmaTest, LemmaDropsIntermediateAncestor) {
    const resolution_lineage *res0, *res1, *res2;
    const goal_lineage *goal0, *goal1;
    build_chain(res0, goal0, res1, goal1, res2);

    lemma l{as_set({res1, res2})};
    EXPECT_EQ(l.get_resolutions(), as_set({res2}));
}

TEST_F(LemmaTest, LemmaIdempotentForLeafOnlyInput) {
    const resolution_lineage *res0, *res1, *res2;
    const goal_lineage *goal0, *goal1;
    build_chain(res0, goal0, res1, goal1, res2);

    lemma l{as_set({res2})};
    EXPECT_EQ(l.get_resolutions(), as_set({res2}));
}

TEST_F(LemmaTest, LemmaVisitedStopsReWalking) {
    const resolution_lineage *res0, *res1, *res2;
    const goal_lineage *goal0, *goal1;
    build_chain(res0, goal0, res1, goal1, res2);

    lemma l{as_set({res2, res1})};
    EXPECT_EQ(l.get_resolutions(), as_set({res2}));
}
