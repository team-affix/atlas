#include <gtest/gtest.h>
#include <unordered_set>
#include "../../../core/hpp/value_objects/lemma.hpp"

struct LemmaTest : public ::testing::Test {
protected:
    expr goal_expr0{expr::var{0}};
    expr rule_head0{expr::var{10}};
    expr rule_head1{expr::var{11}};
    rule rule_idx0{&rule_head0, {}};
    rule rule_idx1{&rule_head1, {}};

    resolution_lineage res0_storage{nullptr, &rule_idx0};
    goal_lineage goal0_storage{nullptr, &goal_expr0};
    resolution_lineage res1_storage{nullptr, &rule_idx0};
    goal_lineage goal1_storage{nullptr, &goal_expr0};
    resolution_lineage res2_storage{nullptr, &rule_idx0};

    void build_chain(resolution_lineage*& res0,
                     goal_lineage*& goal0,
                     resolution_lineage*& res1,
                     goal_lineage*& goal1,
                     resolution_lineage*& res2) {
        res0_storage  = resolution_lineage{nullptr, &rule_idx0};
        goal0_storage = goal_lineage{&res0_storage, &goal_expr0};
        res1_storage  = resolution_lineage{&goal0_storage, &rule_idx0};
        goal1_storage = goal_lineage{&res1_storage, &goal_expr0};
        res2_storage  = resolution_lineage{&goal1_storage, &rule_idx0};

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
    resolution_lineage res0{nullptr, &rule_idx0};
    lemma l{as_set({&res0})};
    EXPECT_EQ(l.get_resolutions(), as_set({&res0}));
}

TEST_F(LemmaTest, LemmaDropsAncestorWhenLeafIncluded) {
    resolution_lineage *res0, *res1, *res2;
    goal_lineage *goal0, *goal1;
    build_chain(res0, goal0, res1, goal1, res2);

    lemma l{as_set({res0, res1, res2})};
    EXPECT_EQ(l.get_resolutions(), as_set({res2}));
}

TEST_F(LemmaTest, LemmaDropsIntermediateAncestor) {
    resolution_lineage *res0, *res1, *res2;
    goal_lineage *goal0, *goal1;
    build_chain(res0, goal0, res1, goal1, res2);

    lemma l{as_set({res1, res2})};
    EXPECT_EQ(l.get_resolutions(), as_set({res2}));
}

TEST_F(LemmaTest, LemmaIdempotentForLeafOnlyInput) {
    resolution_lineage *res0, *res1, *res2;
    goal_lineage *goal0, *goal1;
    build_chain(res0, goal0, res1, goal1, res2);

    lemma l{as_set({res2})};
    EXPECT_EQ(l.get_resolutions(), as_set({res2}));
}

TEST_F(LemmaTest, LemmaPrunesToDeepestResolutionsOnly) {
    resolution_lineage *res0, *res1, *res2;
    goal_lineage *goal0, *goal1;
    build_chain(res0, goal0, res1, goal1, res2);

    lemma l{as_set({res2, res1})};
    EXPECT_EQ(l.get_resolutions(), as_set({res2}));
}
