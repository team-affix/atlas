#include <gtest/gtest.h>
#include "infrastructure/functor_names.hpp"

struct FunctorNamesTest : public ::testing::Test {
protected:
    functor_names names;
};

TEST_F(FunctorNamesTest, ConstructorRegistersNilAndCons) {
    EXPECT_TRUE(names.is_named(k_nil_functor_id));
    EXPECT_TRUE(names.is_named(k_cons_functor_id));
    EXPECT_EQ(names.name(k_nil_functor_id), "nil");
    EXPECT_EQ(names.name(k_cons_functor_id), "cons");
}

TEST_F(FunctorNamesTest, UnnamedIdIsNotNamed) {
    EXPECT_FALSE(names.is_named(k_first_user_functor_id));
}

TEST_F(FunctorNamesTest, SetNameMakesIdNamed) {
    names.set_name(3, "X");
    EXPECT_TRUE(names.is_named(3));
    EXPECT_EQ(names.name(3), "X");
}

TEST_F(FunctorNamesTest, DifferentIdsHaveIndependentNames) {
    names.set_name(5, "a");
    names.set_name(6, "b");
    EXPECT_EQ(names.name(5), "a");
    EXPECT_EQ(names.name(6), "b");
    EXPECT_FALSE(names.is_named(7));
}

TEST_F(FunctorNamesTest, NameOnUnnamedIdThrows) {
    EXPECT_THROW(names.name(k_first_user_functor_id), std::out_of_range);
}

TEST_F(FunctorNamesTest, SetNameDoesNotOverwriteExisting) {
    names.set_name(5, "first");
    names.set_name(5, "second");
    EXPECT_EQ(names.name(5), "first");
}
