#include "../../../doctest/doctest/doctest.h"
#include "../../../core/hpp/bootstrap/resolver.hpp"

TEST_CASE("resolver") {
    bindings b;
    resolver::register_bindings(&b);

    SUBCASE("resolve returns correct object after register_bindings") {
        int x = 7;
        b.bind(x);
        CHECK(&resolver::resolve<int>() == &x);
        CHECK(resolver::resolve<int>() == 7);
    }

    SUBCASE("multiple types resolve through resolver") {
        int x = 1;
        double d = 2.5;
        b.bind(x);
        b.bind(d);
        CHECK(&resolver::resolve<int>() == &x);
        CHECK(&resolver::resolve<double>() == &d);
    }

    resolver::register_bindings(nullptr);
}
