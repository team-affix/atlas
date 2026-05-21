#include <gtest/gtest.h>
#include <unordered_set>
#include "../../../core/hpp/infrastructure/goal_candidates_extractor_visitor.hpp"

TEST(GoalCandidatesExtractorVisitorTest, VisitInsertsRuleIntoSet) {
    expr head0{expr::var{0}};
    expr head1{expr::var{1}};
    rule r0{&head0, {}};
    rule r1{&head1, {}};

    std::unordered_set<const rule*> extracted;
    goal_candidates_extractor_visitor visitor{extracted};

    visitor.visit(&r0);
    visitor.visit(&r1);
    visitor.visit(&r0);

    EXPECT_EQ(extracted.size(), 2u);
    EXPECT_TRUE(extracted.contains(&r0));
    EXPECT_TRUE(extracted.contains(&r1));
}
