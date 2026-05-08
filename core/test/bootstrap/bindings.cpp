#include "../../../doctest/doctest/doctest.h"
#include "../../../core/hpp/bootstrap/bindings.hpp"
#include <stdexcept>

TEST_CASE("bindings") {
    bindings b;
    int x = 42;
    b.bind(x);

    SUBCASE("bind + resolve returns same object by reference") {
        CHECK(&b.resolve<int>() == &x);
        CHECK(b.resolve<int>() == 42);
    }

    SUBCASE("multiple distinct types all resolve correctly") {
        double d = 3.14;
        b.bind(d);
        CHECK(&b.resolve<int>() == &x);
        CHECK(&b.resolve<double>() == &d);
    }

    SUBCASE("resolve with unbound type throws") {
        CHECK_THROWS_AS(b.resolve<double>(), std::out_of_range);
    }

    SUBCASE("bind duplicate type throws") {
        int y = 99;
        CHECK_THROWS_AS(b.bind(y), std::logic_error);
    }
}
