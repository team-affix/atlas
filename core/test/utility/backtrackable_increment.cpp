#include "../../../external/doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/backtrackable_increment.hpp"

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
}
