#include <deque>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_structure_watch_descent.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::ReturnRef;
using ::testing::UnorderedElementsAre;

namespace {

struct MockGetNodeAddedSpecializations {
    MOCK_METHOD((const std::vector<pud_specialization>&), get, (pud_node_id));
};

using test_descent_t = pud_structure_watch_descent<MockGetNodeAddedSpecializations>;
using result_t = test_descent_t::descend_result;

struct PudStructureWatchDescentTest : public ::testing::Test {
    NiceMock<MockGetNodeAddedSpecializations> get_specs;
    std::deque<expr> pool;
    std::vector<pud_specialization> child_specs;
    std::vector<pud_specialization> grandchild_specs;
    std::vector<pud_specialization> sibling_specs;
    pud_node_id child = 1;
    pud_node_id grandchild = 2;
    pud_node_id sibling = 3;
    uint32_t f = 100;
    uint32_t g = 101;
    uint32_t a = 102;

    void SetUp() override {
        ON_CALL(get_specs, get(child)).WillByDefault(ReturnRef(child_specs));
        ON_CALL(get_specs, get(grandchild)).WillByDefault(ReturnRef(grandchild_specs));
        ON_CALL(get_specs, get(sibling)).WillByDefault(ReturnRef(sibling_specs));
    }

    const expr* var(uint32_t index) {
        pool.push_back(expr{.content = expr::var{.index = index}});
        return &pool.back();
    }

    const expr* functor(uint32_t id, std::vector<const expr*> args) {
        pool.push_back(expr{.content = expr::functor{.id = id, .args = std::move(args)}});
        return &pool.back();
    }

    pud_specialization rep_instantiated(uint32_t rep, const expr* value) {
        return pud_specialization{.var_idx = rep, .value = value};
    }

    pud_specialization rep_updated(uint32_t demoted_rep, uint32_t surviving_rep) {
        return pud_specialization{.var_idx = demoted_rep, .value = var(surviving_rep)};
    }

    test_descent_t at_root(const std::vector<const expr*>& watched) {
        return test_descent_t{get_specs, watched};
    }

    static std::vector<pud_mhsw_head_id> changes_at(const test_descent_t& parent, pud_node_id node) {
        return parent.descend(node, {}, {}).structure_changes;
    }
};

// ── root ─────────────────────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentTest, RootHeadIdsAreTheWatchedIndices) {
    child_specs = {rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(at_root({var(0), var(1), var(2)}), child), ElementsAre(1u));
}

TEST_F(PudStructureWatchDescentTest, RootWatchingNothingNeverChanges) {
    child_specs = {rep_instantiated(0, functor(a, {})), rep_updated(2, 1)};
    EXPECT_THAT(changes_at(at_root({}), child), IsEmpty());
}

TEST_F(PudStructureWatchDescentTest, GroundExpressionNeverChanges) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    EXPECT_THAT(changes_at(at_root({functor(f, {functor(a, {})})}), child), IsEmpty());
}

TEST_F(PudStructureWatchDescentTest, EveryRepOfANestedExpressionIsWatched) {
    child_specs = {rep_instantiated(2, functor(a, {}))};
    EXPECT_THAT(changes_at(at_root({functor(f, {var(0), functor(g, {var(1), var(2)})})}), child), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, RepeatedRepInOneExpressionIsOneRep) {
    child_specs = {rep_updated(3, 4)};
    EXPECT_THAT(changes_at(at_root({functor(f, {var(3), var(3)})}), child), IsEmpty());
}

// ── result ───────────────────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentTest, ResultCarriesChangesAddedIdsAndChild) {
    child_specs = {rep_instantiated(0, functor(g, {var(3)}))};
    result_t result = at_root({functor(f, {var(0), var(1)})}).descend(child, {}, {var(4)});
    EXPECT_THAT(result.structure_changes, ElementsAre(0u));
    EXPECT_THAT(result.added_head_ids, ElementsAre(1u));
    grandchild_specs = {rep_instantiated(4, functor(a, {}))};
    EXPECT_THAT(changes_at(result.child_descent, grandchild), ElementsAre(1u));
}

TEST_F(PudStructureWatchDescentTest, NothingChangedAndNothingAddedIsEmpty) {
    result_t result = at_root({var(0)}).descend(child, {}, {});
    EXPECT_THAT(result.structure_changes, IsEmpty());
    EXPECT_THAT(result.added_head_ids, IsEmpty());
}

// ── watching through descend ─────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentTest, AddedIdsFollowTheWatchedOrder) {
    result_t result = at_root({var(0)}).descend(child, {}, {var(1), var(2), var(3)});
    EXPECT_THAT(result.added_head_ids, ElementsAre(1u, 2u, 3u));
}

TEST_F(PudStructureWatchDescentTest, AddedIdsKeepCountingAcrossLevels) {
    result_t at_child = at_root({var(0)}).descend(child, {}, {var(1)});
    result_t at_grandchild = at_child.child_descent.descend(grandchild, {}, {var(2)});
    EXPECT_THAT(at_grandchild.added_head_ids, ElementsAre(2u));
}

TEST_F(PudStructureWatchDescentTest, UnwatchedIdsAreNotReused) {
    result_t result = at_root({var(0), var(1)}).descend(child, {0}, {var(2)});
    EXPECT_THAT(result.added_head_ids, ElementsAre(2u));
}

TEST_F(PudStructureWatchDescentTest, NewlyWatchedIsNotAChangeInItsOwnStep) {
    child_specs = {rep_updated(1, 0)};
    result_t result = at_root({functor(f, {var(0), var(1)})}).descend(child, {}, {functor(g, {var(0)})});
    EXPECT_THAT(result.structure_changes, ElementsAre(0u));
    EXPECT_THAT(result.added_head_ids, ElementsAre(1u));
}

TEST_F(PudStructureWatchDescentTest, NewlyWatchedIsTrackedAtTheNextLevel) {
    result_t at_child = at_root({}).descend(child, {}, {functor(f, {var(3)})});
    grandchild_specs = {rep_instantiated(3, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, NewlyWatchedSharesRepsWithExistingHeads) {
    result_t at_child = at_root({var(0)}).descend(child, {}, {functor(f, {var(0), var(1)})});
    grandchild_specs = {rep_instantiated(0, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), UnorderedElementsAre(0u, 1u));
}

// ── unwatching through descend ───────────────────────────────────────────────

TEST_F(PudStructureWatchDescentTest, UnwatchedHeadIsNotAChangeInTheSameStep) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    EXPECT_THAT(at_root({var(0), var(0)}).descend(child, {0}, {}).structure_changes, ElementsAre(1u));
}

TEST_F(PudStructureWatchDescentTest, UnwatchedHeadStaysGoneAtLaterLevels) {
    result_t at_child = at_root({var(0), var(1)}).descend(child, {0}, {});
    grandchild_specs = {rep_instantiated(0, functor(a, {})), rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(1u));
}

TEST_F(PudStructureWatchDescentTest, UnwatchKeepsOtherHeadsOnSharedReps) {
    result_t at_child = at_root({functor(f, {var(0)}), functor(g, {var(0), var(1)})}).descend(child, {1}, {});
    grandchild_specs = {rep_instantiated(0, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, UnwatchOfAnUnknownIdIsHarmless) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    EXPECT_THAT(at_root({var(0)}).descend(child, {42}, {}).structure_changes, ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, UnwatchAndWatchInTheSameStep) {
    result_t at_child = at_root({var(0)}).descend(child, {0}, {var(0)});
    EXPECT_THAT(at_child.added_head_ids, ElementsAre(1u));
    grandchild_specs = {rep_instantiated(0, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(1u));
}

// ── descend: no change ───────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentTest, DescendLooksUpTheChildSpecializations) {
    EXPECT_CALL(get_specs, get(child)).WillOnce(ReturnRef(child_specs));
    changes_at(at_root({var(0)}), child);
}

TEST_F(PudStructureWatchDescentTest, NoSpecializationsChangesNothing) {
    EXPECT_THAT(changes_at(at_root({functor(f, {var(0), var(1)})}), child), IsEmpty());
}

TEST_F(PudStructureWatchDescentTest, InstantiatingAnUnwatchedRepChangesNothing) {
    child_specs = {rep_instantiated(5, functor(a, {}))};
    EXPECT_THAT(changes_at(at_root({functor(f, {var(0), var(1)})}), child), IsEmpty());
}

TEST_F(PudStructureWatchDescentTest, UpdatingTwoUnwatchedRepsChangesNothing) {
    child_specs = {rep_updated(6, 5)};
    EXPECT_THAT(changes_at(at_root({functor(f, {var(0), var(1)})}), child), IsEmpty());
}

TEST_F(PudStructureWatchDescentTest, UpdatingAWatchedRepToAnOutsideRepChangesNothing) {
    child_specs = {rep_updated(1, 5)};
    EXPECT_THAT(changes_at(at_root({functor(f, {var(0), var(1)})}), child), IsEmpty());
}

TEST_F(PudStructureWatchDescentTest, UpdatingAnOutsideRepToAWatchedRepChangesNothing) {
    child_specs = {rep_updated(5, 1)};
    EXPECT_THAT(changes_at(at_root({functor(f, {var(0), var(1)})}), child), IsEmpty());
}

TEST_F(PudStructureWatchDescentTest, UpdatingRepsOfDifferentHeadsChangesNothing) {
    child_specs = {rep_updated(0, 1)};
    EXPECT_THAT(changes_at(at_root({var(0), var(1)}), child), IsEmpty());
}

// ── descend: rep instantiated ────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentTest, InstantiatingAWatchedRepChangesItsHead) {
    child_specs = {rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(at_root({functor(f, {var(0), var(1)})}), child), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, InstantiatingChangesEveryHeadWatchingThatRep) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    test_descent_t root = at_root({functor(f, {var(0)}), functor(g, {var(0), var(1)}), var(1)});
    EXPECT_THAT(changes_at(root, child), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescentTest, GroundInstantiationLeavesTheOtherRepsWatched) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    result_t at_child = at_root({functor(f, {var(0), var(1)})}).descend(child, {}, {});
    grandchild_specs = {rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, GroundInstantiationLeavesNothingToCollapse) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    result_t at_child = at_root({functor(f, {var(0), var(1)})}).descend(child, {}, {});
    grandchild_specs = {rep_updated(1, 5)};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), IsEmpty());
}

TEST_F(PudStructureWatchDescentTest, InstantiationWatchesTheValuesReps) {
    child_specs = {rep_instantiated(0, functor(g, {var(3), var(4)}))};
    result_t at_child = at_root({functor(f, {var(0), var(1)})}).descend(child, {}, {});
    grandchild_specs = {rep_updated(4, 3)};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, InstantiationWatchesRepsNestedInTheValue) {
    child_specs = {rep_instantiated(0, functor(g, {functor(f, {var(6)}), var(7)}))};
    result_t at_child = at_root({var(0)}).descend(child, {}, {});
    grandchild_specs = {rep_updated(7, 6)};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, InstantiationIntroducingAnAlreadyWatchedRepKeepsWatchingIt) {
    child_specs = {rep_instantiated(0, functor(g, {var(1)}))};
    result_t at_child = at_root({functor(f, {var(0), var(1)})}).descend(child, {}, {});
    EXPECT_THAT(at_child.structure_changes, ElementsAre(0u));
    grandchild_specs = {rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, IntroducedRepsAreWatchedAtTheNextLevel) {
    child_specs = {rep_instantiated(0, functor(g, {var(3)}))};
    result_t at_child = at_root({var(0)}).descend(child, {}, {});
    grandchild_specs = {rep_instantiated(3, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

// ── descend: rep updated ─────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentTest, UpdatingTwoRepsOfTheSameHeadChangesIt) {
    child_specs = {rep_updated(1, 0)};
    EXPECT_THAT(changes_at(at_root({functor(f, {var(0), var(1)})}), child), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, CollapseIsAChangeWhicheverRepSurvives) {
    child_specs = {rep_updated(0, 1)};
    EXPECT_THAT(changes_at(at_root({functor(f, {var(0), var(1)})}), child), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, CollapsedHeadIsWatchedUnderTheSurvivingRep) {
    child_specs = {rep_updated(1, 0)};
    result_t at_child = at_root({functor(f, {var(0), var(1)})}).descend(child, {}, {});
    grandchild_specs = {rep_instantiated(0, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, DemotedRepIsWatchedThroughTheSurvivingRep) {
    child_specs = {rep_updated(4, 1)};
    result_t at_child = at_root({functor(f, {var(4)})}).descend(child, {}, {});
    EXPECT_THAT(at_child.structure_changes, IsEmpty());
    grandchild_specs = {rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, DemotedRepIsNoLongerWatchedUnderItsOldIndex) {
    child_specs = {rep_updated(4, 1)};
    result_t at_child = at_root({functor(f, {var(4)})}).descend(child, {}, {});
    grandchild_specs = {rep_instantiated(4, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), IsEmpty());
}

TEST_F(PudStructureWatchDescentTest, RepUpdateFollowsTheSpecializationNotTheSmallerIndex) {
    child_specs = {rep_updated(1, 4)};
    result_t at_child = at_root({functor(f, {var(1)})}).descend(child, {}, {});
    grandchild_specs = {rep_instantiated(4, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, SurvivingRepHeadsStayInPlace) {
    child_specs = {rep_updated(4, 1)};
    result_t at_child = at_root({functor(f, {var(1)})}).descend(child, {}, {});
    EXPECT_THAT(at_child.structure_changes, IsEmpty());
    grandchild_specs = {rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, MergedClassInstantiationChangesHeadsOfBothSides) {
    child_specs = {rep_updated(0, 1)};
    result_t at_child = at_root({var(0), var(1)}).descend(child, {}, {});
    ASSERT_THAT(at_child.structure_changes, IsEmpty());
    grandchild_specs = {rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescentTest, RepUpdateChangesOnlyTheHeadsThatCollapse) {
    child_specs = {rep_updated(0, 1)};
    test_descent_t root = at_root({functor(f, {var(0), var(1)}), var(1), functor(g, {var(0), var(2)})});
    EXPECT_THAT(changes_at(root, child), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, ChainedRepUpdatesAtOneNodeCollapseAcrossThreeReps) {
    child_specs = {rep_updated(2, 1), rep_updated(1, 0)};
    EXPECT_THAT(changes_at(at_root({functor(f, {var(0), var(2)})}), child), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, SurvivingRepInstantiatedAtTheSameNodeIsDropped) {
    child_specs = {rep_updated(1, 0), rep_instantiated(0, functor(a, {}))};
    result_t at_child = at_root({functor(f, {var(0), var(1)})}).descend(child, {}, {});
    EXPECT_THAT(at_child.structure_changes, ElementsAre(0u));
    grandchild_specs = {rep_instantiated(0, functor(a, {})), rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), IsEmpty());
}

// ── descend: reporting ───────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentTest, HeadIsReportedOncePerDescend) {
    child_specs = {rep_instantiated(0, functor(a, {})), rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(at_root({functor(f, {var(0), var(1)})}), child), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, HeadChangedByRepUpdateAndRepInstantiationIsReportedOnce) {
    child_specs = {rep_updated(1, 0), rep_instantiated(0, functor(a, {}))};
    EXPECT_THAT(changes_at(at_root({functor(f, {var(0), var(1)})}), child), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, HeadChangedAtChildCanChangeAgainAtGrandchild) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    result_t at_child = at_root({functor(f, {var(0), var(1)})}).descend(child, {}, {});
    ASSERT_THAT(at_child.structure_changes, ElementsAre(0u));
    grandchild_specs = {rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

// ── descend: persistence ─────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentTest, ParentIsUntouchedByDescend) {
    test_descent_t parent = at_root({functor(f, {var(0), var(1)})});
    child_specs = {rep_instantiated(0, functor(g, {var(5)}))};
    parent.descend(child, {0}, {var(7)});
    sibling_specs = {rep_instantiated(0, functor(a, {}))};
    result_t at_sibling = parent.descend(sibling, {}, {var(8)});
    EXPECT_THAT(at_sibling.structure_changes, ElementsAre(0u));
    EXPECT_THAT(at_sibling.added_head_ids, ElementsAre(1u));
}

TEST_F(PudStructureWatchDescentTest, SiblingsDescendIndependently) {
    test_descent_t parent = at_root({var(0), var(1)});
    child_specs = {rep_instantiated(0, functor(a, {}))};
    sibling_specs = {rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(parent, child), ElementsAre(0u));
    EXPECT_THAT(changes_at(parent, sibling), ElementsAre(1u));
}

TEST_F(PudStructureWatchDescentTest, DescentIsAssignable) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    test_descent_t current = at_root({var(0), var(1)});
    current = current.descend(child, {}, {}).child_descent;
    grandchild_specs = {rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(current, grandchild), ElementsAre(1u));
}

}  // namespace
