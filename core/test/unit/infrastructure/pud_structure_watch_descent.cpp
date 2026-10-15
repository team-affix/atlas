#include <deque>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_structure_watch_descent.hpp"

using ::testing::_;
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
    std::vector<pud_specialization> no_specs;
    pud_node_id child = 1;
    pud_node_id grandchild = 2;
    pud_node_id sibling = 3;
    uint32_t f = 100;
    uint32_t g = 101;
    uint32_t a = 102;

    void SetUp() override {
        ON_CALL(get_specs, get(_)).WillByDefault(ReturnRef(no_specs));
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

TEST_F(PudStructureWatchDescentTest, CopiedDescentsAreIndependent) {
    test_descent_t original = at_root({var(0), var(1)});
    test_descent_t copy = original;
    child_specs = {rep_instantiated(0, functor(a, {}))};
    copy = copy.descend(child, {1}, {}).child_descent;
    sibling_specs = {rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(original, sibling), ElementsAre(1u));
    EXPECT_THAT(changes_at(copy, sibling), IsEmpty());
}

TEST_F(PudStructureWatchDescentTest, UnwatchInAChildLeavesTheParentWatching) {
    test_descent_t parent = at_root({var(0)});
    parent.descend(child, {0}, {});
    sibling_specs = {rep_instantiated(0, functor(a, {}))};
    EXPECT_THAT(changes_at(parent, sibling), ElementsAre(0u));
}

// ── ordering ─────────────────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentTest, ChangesFollowTheSpecializationOrder) {
    child_specs = {
        rep_instantiated(2, functor(a, {})),
        rep_instantiated(0, functor(a, {})),
        rep_instantiated(1, functor(a, {})),
    };
    EXPECT_THAT(changes_at(at_root({var(0), var(1), var(2)}), child), ElementsAre(2u, 0u, 1u));
}

TEST_F(PudStructureWatchDescentTest, RepeatedHeadKeepsItsFirstPosition) {
    child_specs = {
        rep_instantiated(0, functor(a, {})),
        rep_instantiated(1, functor(a, {})),
        rep_instantiated(2, functor(a, {})),
    };
    test_descent_t root = at_root({functor(f, {var(0), var(2)}), var(1)});
    EXPECT_THAT(changes_at(root, child), ElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescentTest, CollapseReportedAtTheSpecializationThatCausesIt) {
    child_specs = {
        rep_instantiated(3, functor(a, {})),
        rep_updated(1, 0),
    };
    test_descent_t root = at_root({functor(f, {var(0), var(1)}), var(3)});
    EXPECT_THAT(changes_at(root, child), ElementsAre(1u, 0u));
}

// ── unwatch corner cases ─────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentTest, UnwatchingTheSameIdTwiceIsHarmless) {
    child_specs = {rep_instantiated(0, functor(a, {}))};
    result_t result = at_root({var(0), var(0)}).descend(child, {0, 0}, {var(5)});
    EXPECT_THAT(result.structure_changes, ElementsAre(1u));
    EXPECT_THAT(result.added_head_ids, ElementsAre(2u));
}

TEST_F(PudStructureWatchDescentTest, UnwatchingAFutureIdDoesNotAffectTheHeadThatLaterGetsIt) {
    result_t at_child = at_root({var(0)}).descend(child, {3}, {var(1), var(2), var(3)});
    ASSERT_THAT(at_child.added_head_ids, ElementsAre(1u, 2u, 3u));
    grandchild_specs = {rep_instantiated(3, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(3u));
}

TEST_F(PudStructureWatchDescentTest, UnwatchingEveryHeadLeavesNothingToChange) {
    result_t at_child = at_root({var(0), functor(f, {var(0), var(1)}), var(2)}).descend(child, {0, 1, 2}, {});
    grandchild_specs = {rep_instantiated(0, functor(a, {})), rep_updated(2, 1)};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), IsEmpty());
}

TEST_F(PudStructureWatchDescentTest, UnwatchingAGroundHeadIsHarmless) {
    result_t result = at_root({functor(f, {functor(a, {})}), var(0)}).descend(child, {0}, {var(1)});
    EXPECT_THAT(result.added_head_ids, ElementsAre(2u));
    grandchild_specs = {rep_instantiated(0, functor(a, {}))};
    EXPECT_THAT(changes_at(result.child_descent, grandchild), ElementsAre(1u));
}

TEST_F(PudStructureWatchDescentTest, RewatchingAnUnwatchedRepWorks) {
    result_t at_child = at_root({var(0)}).descend(child, {0}, {});
    result_t at_grandchild = at_child.child_descent.descend(grandchild, {}, {functor(f, {var(0)})});
    sibling_specs = {rep_instantiated(0, functor(a, {}))};
    EXPECT_THAT(changes_at(at_grandchild.child_descent, sibling), ElementsAre(1u));
}

TEST_F(PudStructureWatchDescentTest, UnwatchingAHeadThatWasMovedToTheSurvivingRepForgetsIt) {
    child_specs = {rep_updated(4, 1)};
    result_t at_child = at_root({var(4), var(1)}).descend(child, {}, {});
    result_t at_grandchild = at_child.child_descent.descend(grandchild, {0}, {});
    sibling_specs = {rep_instantiated(1, functor(a, {}))};
    EXPECT_THAT(changes_at(at_grandchild.child_descent, sibling), ElementsAre(1u));
}

TEST_F(PudStructureWatchDescentTest, UnwatchingAHeadThatGainedIntroducedRepsForgetsThemToo) {
    child_specs = {rep_instantiated(0, functor(g, {var(5), var(6)}))};
    result_t at_child = at_root({var(0), var(6)}).descend(child, {}, {});
    result_t at_grandchild = at_child.child_descent.descend(grandchild, {0}, {});
    sibling_specs = {rep_instantiated(5, functor(a, {})), rep_instantiated(6, functor(a, {}))};
    EXPECT_THAT(changes_at(at_grandchild.child_descent, sibling), ElementsAre(1u));
}

// ── watch corner cases ───────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentTest, WatchingTheSameExpressionTwiceGivesTwoHeads) {
    const expr* watched = functor(f, {var(0)});
    result_t at_child = at_root({}).descend(child, {}, {watched, watched});
    ASSERT_THAT(at_child.added_head_ids, ElementsAre(0u, 1u));
    grandchild_specs = {rep_instantiated(0, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescentTest, RootWatchingTheSameExpressionTwiceGivesTwoHeads) {
    const expr* watched = functor(f, {var(0), var(1)});
    child_specs = {rep_updated(1, 0)};
    EXPECT_THAT(changes_at(at_root({watched, watched}), child), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescentTest, WatchingAfterACollapseStartsFromTheCollapsedStructure) {
    child_specs = {rep_updated(1, 0)};
    result_t at_child = at_root({functor(f, {var(0), var(1)})}).descend(child, {}, {functor(g, {var(0), var(0)})});
    grandchild_specs = {rep_updated(2, 0)};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), IsEmpty());
}

TEST_F(PudStructureWatchDescentTest, WatchingOnlyAtADeepLevelWorks) {
    test_descent_t at = at_root({});
    for (pud_node_id level_node = 10; level_node < 20; ++level_node)
        at = at.descend(level_node, {}, {}).child_descent;
    result_t deep = at.descend(child, {}, {var(9)});
    ASSERT_THAT(deep.added_head_ids, ElementsAre(0u));
    grandchild_specs = {rep_instantiated(9, functor(a, {}))};
    EXPECT_THAT(changes_at(deep.child_descent, grandchild), ElementsAre(0u));
}

// ── interaction between heads ────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentTest, OneRepUpdateCollapsesEveryHeadContainingBothReps) {
    child_specs = {rep_updated(1, 0)};
    test_descent_t root = at_root({functor(f, {var(0), var(1)}), functor(g, {var(1), var(0)})});
    EXPECT_THAT(changes_at(root, child), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescentTest, ThreeWayMergeCollapsesTwice) {
    child_specs = {rep_updated(1, 0)};
    result_t at_child = at_root({functor(f, {var(0), var(1), var(2)})}).descend(child, {}, {});
    ASSERT_THAT(at_child.structure_changes, ElementsAre(0u));
    grandchild_specs = {rep_updated(2, 0)};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, InstantiatingIntoAnotherHeadsRepLeavesThatHeadAlone) {
    child_specs = {rep_instantiated(0, functor(g, {var(5)}))};
    result_t at_child = at_root({var(0), var(5)}).descend(child, {}, {});
    EXPECT_THAT(at_child.structure_changes, ElementsAre(0u));
    grandchild_specs = {rep_instantiated(5, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), UnorderedElementsAre(0u, 1u));
}

TEST_F(PudStructureWatchDescentTest, IntroducedRepThatMeetsAnExistingRepCollapsesLater) {
    child_specs = {rep_instantiated(0, functor(g, {var(2)}))};
    result_t at_child = at_root({functor(f, {var(0), var(1)})}).descend(child, {}, {});
    grandchild_specs = {rep_updated(2, 1)};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, RenamingChainWithoutCollapseNeverReports) {
    child_specs = {rep_updated(5, 4), rep_updated(4, 3), rep_updated(3, 2)};
    result_t at_child = at_root({functor(f, {var(5), var(0)})}).descend(child, {}, {});
    EXPECT_THAT(at_child.structure_changes, IsEmpty());
    grandchild_specs = {rep_instantiated(2, functor(a, {}))};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, RepUpdateOfAnUnseenSurvivingRepCreatesItsEntry) {
    child_specs = {rep_updated(3, 7)};
    result_t at_child = at_root({var(3)}).descend(child, {}, {});
    grandchild_specs = {rep_updated(7, 0)};
    result_t at_grandchild = at_child.child_descent.descend(grandchild, {}, {});
    EXPECT_THAT(at_grandchild.structure_changes, IsEmpty());
    sibling_specs = {rep_instantiated(0, functor(a, {}))};
    EXPECT_THAT(changes_at(at_grandchild.child_descent, sibling), ElementsAre(0u));
}

// ── stress ───────────────────────────────────────────────────────────────────

TEST_F(PudStructureWatchDescentTest, ManyHeadsOnDistinctRepsAreReportedInSpecializationOrder) {
    const uint32_t head_count = 2000;
    std::vector<const expr*> watched;
    std::vector<pud_mhsw_head_id> expected;
    for (uint32_t index = 0; index < head_count; ++index) {
        watched.push_back(var(index));
        child_specs.push_back(rep_instantiated(head_count - 1 - index, functor(a, {})));
        expected.push_back(head_count - 1 - index);
    }
    EXPECT_EQ(changes_at(at_root(watched), child), expected);
}

TEST_F(PudStructureWatchDescentTest, ManyHeadsOnOneRepAreEachReportedOnce) {
    const uint32_t head_count = 2000;
    std::vector<const expr*> watched(head_count, var(0));
    child_specs = {rep_instantiated(0, functor(a, {})), rep_instantiated(0, functor(a, {}))};
    std::vector<pud_mhsw_head_id> changes = changes_at(at_root(watched), child);
    std::sort(changes.begin(), changes.end());
    ASSERT_EQ(changes.size(), head_count);
    for (uint32_t index = 0; index < head_count; ++index)
        EXPECT_EQ(changes.at(index), index);
}

TEST_F(PudStructureWatchDescentTest, LongRenamingChainAtOneNodeCollapsesOnlyAtTheEnd) {
    const uint32_t chain_length = 1500;
    for (uint32_t rep = chain_length; rep > 1; --rep)
        child_specs.push_back(rep_updated(rep, rep - 1));
    result_t at_child = at_root({functor(f, {var(0), var(chain_length)})}).descend(child, {}, {});
    EXPECT_THAT(at_child.structure_changes, IsEmpty());
    grandchild_specs = {rep_updated(1, 0)};
    EXPECT_THAT(changes_at(at_child.child_descent, grandchild), ElementsAre(0u));
}

TEST_F(PudStructureWatchDescentTest, LongRenamingChainAcrossLevelsCollapsesOnlyAtTheEnd) {
    const uint32_t chain_length = 500;
    std::vector<std::vector<pud_specialization>> level_specs(chain_length);
    test_descent_t at = at_root({functor(f, {var(0), var(chain_length)})});
    for (uint32_t step = 0; step < chain_length; ++step) {
        const uint32_t demoted = chain_length - step;
        const pud_node_id level_node = 100 + step;
        level_specs.at(step) = {rep_updated(demoted, demoted - 1)};
        ON_CALL(get_specs, get(level_node)).WillByDefault(ReturnRef(level_specs.at(step)));
        result_t result = at.descend(level_node, {}, {});
        const bool last_step = step + 1 == chain_length;
        if (last_step) {
            EXPECT_THAT(result.structure_changes, ElementsAre(0u));
        } else {
            EXPECT_THAT(result.structure_changes, IsEmpty()) << "step " << step;
        }
        at = result.child_descent;
    }
}

TEST_F(PudStructureWatchDescentTest, DeepPathWithRollingWatchAndUnwatch) {
    const uint32_t depth = 1000;
    test_descent_t at = at_root({var(0)});
    for (uint32_t step = 1; step <= depth; ++step) {
        result_t result = at.descend(10000 + step, {step - 1}, {var(step)});
        ASSERT_THAT(result.added_head_ids, ElementsAre(step));
        at = result.child_descent;
    }
    grandchild_specs = {};
    for (uint32_t rep = 0; rep <= depth; ++rep)
        grandchild_specs.push_back(rep_instantiated(rep, functor(a, {})));
    EXPECT_THAT(changes_at(at, grandchild), ElementsAre(depth));
}

TEST_F(PudStructureWatchDescentTest, WideBranchingFromOneParentStaysIndependent) {
    const uint32_t branch_count = 300;
    std::vector<const expr*> watched;
    for (uint32_t index = 0; index < branch_count; ++index)
        watched.push_back(var(index));
    test_descent_t parent = at_root(watched);
    std::vector<std::vector<pud_specialization>> branch_specs(branch_count);
    for (uint32_t branch = 0; branch < branch_count; ++branch) {
        const pud_node_id branch_node = 1000 + branch;
        branch_specs.at(branch) = {rep_instantiated(branch, functor(a, {}))};
        ON_CALL(get_specs, get(branch_node)).WillByDefault(ReturnRef(branch_specs.at(branch)));
        EXPECT_THAT(changes_at(parent, branch_node), ElementsAre(branch));
    }
}

}  // namespace
