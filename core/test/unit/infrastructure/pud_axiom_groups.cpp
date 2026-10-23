#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_axiom_groups.hpp"

TEST(PudAxiomGroupsTest, VisibleRootsIncludeTheGroupAndItsDependenciesInOrder) {
    pud_axiom_groups groups;
    const pud_axiom_group_id base = groups.create_group();
    const pud_axiom_group_id derived = groups.create_group();
    groups.add_dependency(derived, base);
    groups.add_member(base, 1);
    groups.add_member(base, 2);
    groups.add_member(derived, 10);
    groups.store_node_group(100, derived);
    groups.store_node_group(1, base);
    EXPECT_EQ(groups.axiom_roots(100), (std::vector<pud_node_id>{1, 2, 10}));
}

TEST(PudAxiomGroupsTest, SiblingGroupsDoNotShareAxioms) {
    pud_axiom_groups groups;
    const pud_axiom_group_id base = groups.create_group();
    const pud_axiom_group_id left = groups.create_group();
    const pud_axiom_group_id right = groups.create_group();
    groups.add_dependency(left, base);
    groups.add_dependency(right, base);
    groups.add_member(base, 1);
    groups.add_member(left, 10);
    groups.add_member(right, 20);
    groups.store_node_group(50, left);
    groups.store_node_group(60, right);
    EXPECT_EQ(groups.axiom_roots(50), (std::vector<pud_node_id>{1, 10}));
    EXPECT_EQ(groups.axiom_roots(60), (std::vector<pud_node_id>{1, 20}));
}

TEST(PudAxiomGroupsTest, AGroupWithNoDependenciesSeesOnlyItsOwnMembers) {
    pud_axiom_groups groups;
    const pud_axiom_group_id lone = groups.create_group();
    groups.add_member(lone, 5);
    groups.add_member(lone, 6);
    groups.store_node_group(9, lone);
    EXPECT_EQ(groups.axiom_roots(9), (std::vector<pud_node_id>{5, 6}));
}

TEST(PudAxiomGroupsTest, TransitiveDependenciesFlattenInDependencyFirstOrder) {
    pud_axiom_groups groups;
    const pud_axiom_group_id bottom = groups.create_group();
    const pud_axiom_group_id middle = groups.create_group();
    const pud_axiom_group_id top = groups.create_group();
    groups.add_dependency(middle, bottom);
    groups.add_dependency(top, middle);
    groups.add_member(bottom, 1);
    groups.add_member(middle, 2);
    groups.add_member(top, 3);
    groups.store_node_group(100, top);
    EXPECT_EQ(groups.axiom_roots(100), (std::vector<pud_node_id>{1, 2, 3}));
}

TEST(PudAxiomGroupsTest, DiamondDependenciesListEachGroupOnce) {
    pud_axiom_groups groups;
    const pud_axiom_group_id base = groups.create_group();
    const pud_axiom_group_id left = groups.create_group();
    const pud_axiom_group_id right = groups.create_group();
    const pud_axiom_group_id top = groups.create_group();
    groups.add_dependency(left, base);
    groups.add_dependency(right, base);
    groups.add_dependency(top, left);
    groups.add_dependency(top, right);
    groups.add_member(base, 1);
    groups.add_member(left, 2);
    groups.add_member(right, 3);
    groups.add_member(top, 4);
    groups.store_node_group(50, top);
    const std::vector<pud_node_id> visible = groups.axiom_roots(50);
    EXPECT_EQ(visible.size(), 4u);
    EXPECT_EQ(visible.back(), 4u);
    EXPECT_EQ(visible.at(0), 1u);
}

TEST(PudAxiomGroupsTest, DuplicateDependenciesAreIgnored) {
    pud_axiom_groups groups;
    const pud_axiom_group_id base = groups.create_group();
    const pud_axiom_group_id derived = groups.create_group();
    groups.add_dependency(derived, base);
    groups.add_dependency(derived, base);
    groups.add_member(base, 1);
    groups.add_member(derived, 2);
    groups.store_node_group(9, derived);
    EXPECT_EQ(groups.axiom_roots(9), (std::vector<pud_node_id>{1, 2}));
}

TEST(PudAxiomGroupsTest, ACyclicDependencyIsRejectedInDebug) {
    pud_axiom_groups groups;
    const pud_axiom_group_id first = groups.create_group();
    const pud_axiom_group_id second = groups.create_group();
    groups.add_dependency(second, first);
    groups.add_member(first, 10);
    groups.add_member(second, 20);
    groups.store_node_group(1, first);
    groups.store_node_group(2, second);
    EXPECT_THROW(groups.add_dependency(first, second), std::logic_error);
    EXPECT_EQ(groups.axiom_roots(2), (std::vector<pud_node_id>{10, 20}));
    EXPECT_EQ(groups.axiom_roots(1), (std::vector<pud_node_id>{10}));
}

TEST(PudAxiomGroupsTest, AddingAMemberRecomputesVisibleRoots) {
    pud_axiom_groups groups;
    const pud_axiom_group_id group = groups.create_group();
    groups.add_member(group, 1);
    groups.store_node_group(5, group);
    EXPECT_EQ(groups.axiom_roots(5), (std::vector<pud_node_id>{1}));
    groups.add_member(group, 2);
    EXPECT_EQ(groups.axiom_roots(5), (std::vector<pud_node_id>{1, 2}));
}

TEST(PudAxiomGroupsTest, NodeGroupRoundTripsThroughTheStore) {
    pud_axiom_groups groups;
    const pud_axiom_group_id group = groups.create_group();
    groups.store_node_group(42, group);
    EXPECT_EQ(groups.node_group(42).value, group.value);
}

TEST(PudAxiomGroupsTest, AnEmptyGroupYieldsNoVisibleAxiomRoots) {
    pud_axiom_groups groups;
    const pud_axiom_group_id empty = groups.create_group();
    groups.store_node_group(3, empty);
    EXPECT_TRUE(groups.axiom_roots(3).empty());
}

TEST(PudAxiomGroupsTest, DeepChainAccumulatesEveryAncestorGroup) {
    pud_axiom_groups groups;
    pud_axiom_group_id previous = groups.create_group();
    groups.add_member(previous, 100);
    pud_axiom_group_id tip = previous;
    for (size_t layer = 1; layer < 8; ++layer) {
        const pud_axiom_group_id next = groups.create_group();
        groups.add_dependency(next, tip);
        groups.add_member(next, 100 + layer);
        tip = next;
    }
    groups.store_node_group(1, tip);
    std::vector<pud_node_id> expected;
    for (size_t layer = 0; layer < 8; ++layer)
        expected.push_back(100 + layer);
    EXPECT_EQ(groups.axiom_roots(1), expected);
}
