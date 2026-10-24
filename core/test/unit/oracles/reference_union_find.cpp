#include <gtest/gtest.h>
#include "reference_union_find.hpp"

// The reference variable classes other tests check bind maps against, checked on hand-worked cases.
struct ReferenceVariableClassesTest : public ::testing::Test {
    reference_variable_classes classes;
};

TEST_F(ReferenceVariableClassesTest, AnUntouchedVariableIsItsOwnClass) {
    EXPECT_EQ(classes.min_index(5), 5u);
    EXPECT_EQ(classes.class_size(5), 1u);
    EXPECT_TRUE(classes.same_class(5, 5));
}

TEST_F(ReferenceVariableClassesTest, ATouchedVariableIsASingletonClass) {
    classes.touch(6);
    EXPECT_EQ(classes.min_index(6), 6u);
    EXPECT_EQ(classes.class_size(6), 1u);
}

TEST_F(ReferenceVariableClassesTest, EquatingTwoVariablesNamesTheClassByTheSmallerIndex) {
    classes.equate(7, 3);
    EXPECT_EQ(classes.min_index(7), 3u);
    EXPECT_EQ(classes.min_index(3), 3u);
    EXPECT_EQ(classes.class_size(7), 2u);
    EXPECT_TRUE(classes.same_class(3, 7));
}

TEST_F(ReferenceVariableClassesTest, EquatingIsTheSameInEitherOrder) {
    classes.equate(3, 7);
    EXPECT_EQ(classes.min_index(7), 3u);
    EXPECT_EQ(classes.class_size(3), 2u);
}

TEST_F(ReferenceVariableClassesTest, EquatingAcrossClassesMergesThemWhole) {
    classes.equate(4, 9);
    classes.equate(2, 9);
    EXPECT_EQ(classes.min_index(4), 2u);
    EXPECT_EQ(classes.min_index(9), 2u);
    EXPECT_EQ(classes.class_size(4), 3u);
    EXPECT_TRUE(classes.same_class(2, 4));
}

TEST_F(ReferenceVariableClassesTest, EquatingWithinOneClassChangesNothing) {
    classes.equate(3, 7);
    classes.equate(7, 3);
    EXPECT_EQ(classes.class_size(3), 2u);
    EXPECT_EQ(classes.min_index(7), 3u);
}

TEST_F(ReferenceVariableClassesTest, SeparateClassesStaySeparate) {
    classes.equate(1, 2);
    classes.equate(3, 4);
    EXPECT_FALSE(classes.same_class(1, 3));
    EXPECT_FALSE(classes.same_class(2, 4));
    EXPECT_EQ(classes.class_size(1), 2u);
    EXPECT_EQ(classes.class_size(4), 2u);
}
