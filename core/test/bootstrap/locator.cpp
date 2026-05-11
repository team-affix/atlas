#include "../../../doctest/doctest/doctest.h"
#include "../../../core/hpp/bootstrap/locator.hpp"

TEST_CASE("locator") {
    bindings b;
    locator::register_bindings(&b);

    SUBCASE("locate returns correct object after register_bindings") {
        int x = 7;
        b.bind(x);
        CHECK(&locator::locate<int>() == &x);
        CHECK(locator::locate<int>() == 7);
    }

    SUBCASE("multiple types locate through locator") {
        int x = 1;
        double d = 2.5;
        b.bind(x);
        b.bind(d);
        CHECK(&locator::locate<int>() == &x);
        CHECK(&locator::locate<double>() == &d);
    }

    locator::register_bindings(nullptr);
}
