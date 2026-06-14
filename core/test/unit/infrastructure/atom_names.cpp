#include <gtest/gtest.h>
#include "infrastructure/atom_names.hpp"

struct AtomNamesTest : public ::testing::Test {
protected:
    atom_names names;
};

TEST_F(AtomNamesTest, ConstructorRegistersNilAndCons) {
    EXPECT_TRUE(names.is_named(k_nil_atom_id));
    EXPECT_TRUE(names.is_named(k_cons_atom_id));
    EXPECT_EQ(names.name(k_nil_atom_id), "nil");
    EXPECT_EQ(names.name(k_cons_atom_id), "cons");
}

TEST_F(AtomNamesTest, UnnamedIdIsNotNamed) {
    EXPECT_FALSE(names.is_named(k_first_user_atom_id));
}

TEST_F(AtomNamesTest, SetNameMakesIdNamed) {
    names.set_name(3, "X");
    EXPECT_TRUE(names.is_named(3));
    EXPECT_EQ(names.name(3), "X");
}

TEST_F(AtomNamesTest, DifferentIdsHaveIndependentNames) {
    names.set_name(5, "a");
    names.set_name(6, "b");
    EXPECT_EQ(names.name(5), "a");
    EXPECT_EQ(names.name(6), "b");
    EXPECT_FALSE(names.is_named(7));
}

TEST_F(AtomNamesTest, NameOnUnnamedIdThrows) {
    EXPECT_THROW(names.name(k_first_user_atom_id), std::out_of_range);
}

TEST_F(AtomNamesTest, SetNameDoesNotOverwriteExisting) {
    names.set_name(5, "first");
    names.set_name(5, "second");
    EXPECT_EQ(names.name(5), "first");
}
