// dbuct_bind_map frame journaling: insert/assign undo via pop_frame.

#include <gtest/gtest.h>
#include "infrastructure/dbuct_bind_map.hpp"
#include "infrastructure/globalizer.hpp"
#include "functor_fixture.hpp"
#include "value_objects/expr.hpp"

struct DbuctBindMapTest : public ::testing::Test {
    test_functors functors;
    globalizer g;
    dbuct_bind_map<globalizer> map{g};
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr func{expr::functor{functors.id("f"), {}}};
};

TEST_F(DbuctBindMapTest, PopRevertsInsert) {
    map.push_frame();
    map.bind(0u, {&func, 0});
    map.pop_frame();

    framed_expr query{&var0, 0};
    EXPECT_EQ(map.whnf(query).skeleton, &var0);
}

TEST_F(DbuctBindMapTest, PopRevertsAssignAfterPathCompression) {
    map.push_frame();
    map.bind(1u, {&var0, 0});
    map.bind(0u, {&func, 0});
    (void)map.whnf({&var0, 0});
    map.pop_frame();

    framed_expr query{&var0, 0};
    EXPECT_EQ(map.whnf(query).skeleton, &var0);
    EXPECT_EQ(map.whnf({&var1, 0}).skeleton, &var1);
}
