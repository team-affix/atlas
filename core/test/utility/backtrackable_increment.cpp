#include "../../../doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/backtrackable_increment.hpp"
#include "../../../core/hpp/utility/trail.hpp"
#include <memory>

TEST_CASE("backtrackable_increment") {
    int x = 5;
    backtrackable_increment<int> m;
    m.capture(x);

    SUBCASE("invoke increments") {
        m.invoke();
        CHECK(x == 6);

        SUBCASE("backtrack decrements back") {
            m.backtrack();
            CHECK(x == 5);
        }
    }

    SUBCASE("repeated invocations each increment by 1") {
        trail t;
        t.push();

        for (int expected = 6; expected <= 8; ++expected) {
            auto mi = std::make_unique<backtrackable_increment<int>>();
            mi->capture(x);
            mi->invoke();
            t.log(std::move(mi));
            CHECK(x == expected);
        }

        SUBCASE("trail pop reverses all increments") {
            t.pop();
            CHECK(x == 5);
        }
    }
}
