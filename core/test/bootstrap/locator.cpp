#include "../../../doctest/doctest/doctest.h"
#include "../../../core/hpp/bootstrap/locator.hpp"

TEST_CASE("locator") {
    bindings b;
    locator::register_bindings(&b);

    SUBCASE("resolve returns correct object after register_bindings") {
        int x = 7;
        b.bind(x);
        CHECK(&locator::resolve<int>() == &x);
        CHECK(locator::resolve<int>() == 7);
    }

    SUBCASE("multiple types resolve through locator") {
        int x = 1;
        double d = 2.5;
        b.bind(x);
        b.bind(d);
        CHECK(&locator::resolve<int>() == &x);
        CHECK(&locator::resolve<double>() == &d);
    }

    locator::register_bindings(nullptr);
}
