#include <gtest/gtest.h>
#include "value_objects/moduled_expr_hash.hpp"

namespace {

struct ModuledExprHashTest : public ::testing::Test {
    expr a{expr::functor{1, {}}};
    expr b{expr::functor{2, {}}};
    moduled_expr_hash hash;
};

}

TEST_F(ModuledExprHashTest, EqualTermsHashEqual) {
    EXPECT_EQ(hash(moduled_expr{&a, 3, 7}), hash(moduled_expr{&a, 3, 7}));
}

TEST_F(ModuledExprHashTest, EachFieldContributesToTheHash) {
    const size_t base = hash(moduled_expr{&a, 3, 7});
    EXPECT_NE(base, hash(moduled_expr{&b, 3, 7}));
    EXPECT_NE(base, hash(moduled_expr{&a, 4, 7}));
    EXPECT_NE(base, hash(moduled_expr{&a, 3, 8}));
}
