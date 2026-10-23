#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/pud_axiom_groups.hpp"

struct PudAxiomGroupsTest : public ::testing::Test {
    pud_axiom_groups groups;
};

TEST_F(PudAxiomGroupsTest, VisibleRootsIncludeTheGroupAndItsDependenciesInOrder) {
    const pud_axiom_group_id base = groups.create_group();
    const pud_axiom_group_id derived = groups.create_group();
    groups.add_dependency(derived, base);
    groups.add_member(base, 1);
    groups.add_member(base, 2);
    groups.add_member(derived, 10);
    EXPECT_EQ(groups.axiom_roots(derived), (std::vector<pud_node_id>{1, 2, 10}));
    EXPECT_EQ(groups.axiom_roots(base), (std::vector<pud_node_id>{1, 2}));
}

TEST_F(PudAxiomGroupsTest, SiblingGroupsDoNotShareAxioms) {
    const pud_axiom_group_id base = groups.create_group();
    const pud_axiom_group_id left = groups.create_group();
    const pud_axiom_group_id right = groups.create_group();
    groups.add_dependency(left, base);
    groups.add_dependency(right, base);
    groups.add_member(base, 1);
    groups.add_member(left, 10);
    groups.add_member(right, 20);
    EXPECT_EQ(groups.axiom_roots(left), (std::vector<pud_node_id>{1, 10}));
    EXPECT_EQ(groups.axiom_roots(right), (std::vector<pud_node_id>{1, 20}));
}

TEST_F(PudAxiomGroupsTest, AGroupWithNoDependenciesSeesOnlyItsOwnMembers) {
    const pud_axiom_group_id lone = groups.create_group();
    groups.add_member(lone, 5);
    groups.add_member(lone, 6);
    EXPECT_EQ(groups.axiom_roots(lone), (std::vector<pud_node_id>{5, 6}));
}

TEST_F(PudAxiomGroupsTest, TransitiveDependenciesFlattenInDependencyFirstOrder) {
    const pud_axiom_group_id bottom = groups.create_group();
    const pud_axiom_group_id middle = groups.create_group();
    const pud_axiom_group_id top = groups.create_group();
    groups.add_dependency(middle, bottom);
    groups.add_dependency(top, middle);
    groups.add_member(bottom, 1);
    groups.add_member(middle, 2);
    groups.add_member(top, 3);
    EXPECT_EQ(groups.axiom_roots(top), (std::vector<pud_node_id>{1, 2, 3}));
}

TEST_F(PudAxiomGroupsTest, DiamondDependenciesListEachGroupOnceDependenciesFirst) {
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
    EXPECT_EQ(groups.axiom_roots(top), (std::vector<pud_node_id>{1, 2, 3, 4}));
}

TEST_F(PudAxiomGroupsTest, DuplicateDependenciesAreIgnored) {
    const pud_axiom_group_id base = groups.create_group();
    const pud_axiom_group_id derived = groups.create_group();
    groups.add_dependency(derived, base);
    groups.add_dependency(derived, base);
    groups.add_member(base, 1);
    groups.add_member(derived, 2);
    EXPECT_EQ(groups.axiom_roots(derived), (std::vector<pud_node_id>{1, 2}));
}

TEST_F(PudAxiomGroupsTest, ACyclicDependencyIsRejectedInDebug) {
    const pud_axiom_group_id first = groups.create_group();
    const pud_axiom_group_id second = groups.create_group();
    groups.add_dependency(second, first);
    groups.add_member(first, 10);
    groups.add_member(second, 20);
    EXPECT_THROW(groups.add_dependency(first, second), std::logic_error);
    EXPECT_EQ(groups.axiom_roots(second), (std::vector<pud_node_id>{10, 20}));
    EXPECT_EQ(groups.axiom_roots(first), (std::vector<pud_node_id>{10}));
}

TEST_F(PudAxiomGroupsTest, AGroupWhoseRootsWereReadCannotGainMembersInDebug) {
    const pud_axiom_group_id group = groups.create_group();
    groups.add_member(group, 1);
    EXPECT_EQ(groups.axiom_roots(group), (std::vector<pud_node_id>{1}));
    EXPECT_THROW(groups.add_member(group, 2), std::logic_error);
}

TEST_F(PudAxiomGroupsTest, AGroupSeenByAReadGroupCannotGainMembersInDebug) {
    const pud_axiom_group_id base = groups.create_group();
    const pud_axiom_group_id derived = groups.create_group();
    groups.add_dependency(derived, base);
    groups.axiom_roots(derived);
    EXPECT_THROW(groups.add_member(base, 1), std::logic_error);
}

TEST_F(PudAxiomGroupsTest, AReadGroupCannotGainDependenciesInDebug) {
    const pud_axiom_group_id base = groups.create_group();
    const pud_axiom_group_id derived = groups.create_group();
    groups.axiom_roots(derived);
    EXPECT_THROW(groups.add_dependency(derived, base), std::logic_error);
}

TEST_F(PudAxiomGroupsTest, ReadingAGroupLeavesTheGroupsThatDependOnItOpen) {
    const pud_axiom_group_id base = groups.create_group();
    const pud_axiom_group_id derived = groups.create_group();
    groups.add_dependency(derived, base);
    groups.add_member(base, 1);
    groups.axiom_roots(base);
    groups.add_member(derived, 2);
    EXPECT_EQ(groups.axiom_roots(derived), (std::vector<pud_node_id>{1, 2}));
}

TEST_F(PudAxiomGroupsTest, AnEmptyGroupYieldsNoVisibleAxiomRoots) {
    const pud_axiom_group_id empty = groups.create_group();
    EXPECT_TRUE(groups.axiom_roots(empty).empty());
}

TEST_F(PudAxiomGroupsTest, DeepChainAccumulatesEveryGroupItDependsOn) {
    pud_axiom_group_id tip = groups.create_group();
    groups.add_member(tip, 100);
    for (size_t layer = 1; layer < 8; ++layer) {
        const pud_axiom_group_id next = groups.create_group();
        groups.add_dependency(next, tip);
        groups.add_member(next, 100 + layer);
        tip = next;
    }
    std::vector<pud_node_id> expected;
    for (size_t layer = 0; layer < 8; ++layer)
        expected.push_back(100 + layer);
    EXPECT_EQ(groups.axiom_roots(tip), expected);
}
