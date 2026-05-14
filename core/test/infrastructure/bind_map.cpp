#include "../../../doctest/doctest/doctest.h"
#include "../../../core/hpp/infrastructure/bind_map.hpp"

TEST_CASE("bind_map") {
    bind_map bm;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr func{expr::functor{"f", {}}};

    SUBCASE("whnf of unbound variable returns itself") {
        CHECK(bm.whnf(&var0) == &var0);
    }

    SUBCASE("whnf of functor returns itself") {
        CHECK(bm.whnf(&func) == &func);
    }

    SUBCASE("whnf of bound var returns bound expr") {
        bm.bind(0, &func);
        CHECK(bm.whnf(&var0) == &func);
    }

    SUBCASE("whnf chain var0->var1->functor returns functor") {
        bm.bind(0, &var1);
        bm.bind(1, &func);
        CHECK(bm.whnf(&var0) == &func);
    }

    SUBCASE("path compression: after chain resolution var0 points directly to functor") {
        bm.bind(0, &var1);
        bm.bind(1, &func);
        bm.whnf(&var0);
        // var0 should now be compressed to point directly at func;
        // a second whnf must still return func
        CHECK(bm.whnf(&var0) == &func);
    }

    SUBCASE("bind overwrites an existing binding") {
        bm.bind(0, &func);
        bm.bind(0, &var1);
        CHECK(bm.whnf(&var0) == &var1);
    }

    SUBCASE("whnf does not recurse into functor arguments") {
        expr fa{expr::functor{"f", {&var0}}};
        bm.bind(0, &func);
        // whnf is weak-head: stops at the outermost functor, does not normalise args
        CHECK(bm.whnf(&fa) == &fa);
    }

    SUBCASE("chain of length 3 collapses with path compression") {
        bm.bind(0, &var1);
        bm.bind(1, &var2);
        bm.bind(2, &func);
        CHECK(bm.whnf(&var0) == &func);
        // after resolution all intermediate vars should compress to func
        CHECK(bm.whnf(&var1) == &func);
        CHECK(bm.whnf(&var2) == &func);
    }
}
