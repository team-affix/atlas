// resolution_memory stores resolutions used in a conflict and derives lemmas with the
// same ancestor-pruning policy as decision_memory. Tests cover empty/clear states,
// multi-insert derivation, and deepest-resolution-only pruning on nested lineages.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_set>
#include "../../../core/hpp/infrastructure/resolution_memory.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

struct ResolutionMemoryTest : public ::testing::Test {
    resolution_memory mem;

    expr goal_expr0{expr::var{0}};
    expr head0{expr::var{10}};
    expr head1{expr::var{11}};
    rule rule0{&head0, {}};
    rule rule1{&head1, {}};
    resolution_lineage rl0{nullptr, &rule0};
    resolution_lineage rl1{nullptr, &rule1};
};

TEST_F(ResolutionMemoryTest, DeriveLemmaEmptyAfterClear) {
    mem.insert(&rl0);
    mem.insert(&rl1);
    mem.clear();

    EXPECT_THAT(mem.derive_lemma().get_resolutions(), IsEmpty());
}

TEST_F(ResolutionMemoryTest, DeriveLemmaEmptyWithNoInsertions) {
    EXPECT_THAT(mem.derive_lemma().get_resolutions(), IsEmpty());
}

TEST_F(ResolutionMemoryTest, DeriveLemmaContainsInsertedResolutions) {
    mem.insert(&rl0);
    mem.insert(&rl1);

    EXPECT_THAT(mem.derive_lemma().get_resolutions(),
        UnorderedElementsAre(&rl0, &rl1));
}

TEST_F(ResolutionMemoryTest, DeriveLemmaPrunesAncestorResolutions) {
    resolution_lineage res0{nullptr, &rule0};
    goal_lineage goal0{&res0, &goal_expr0};
    resolution_lineage res1{&goal0, &rule0};
    goal_lineage goal1{&res1, &goal_expr0};
    resolution_lineage res2{&goal1, &rule0};

    mem.insert(&res0);
    mem.insert(&res1);
    mem.insert(&res2);

    EXPECT_THAT(mem.derive_lemma().get_resolutions(), UnorderedElementsAre(&res2));
}
