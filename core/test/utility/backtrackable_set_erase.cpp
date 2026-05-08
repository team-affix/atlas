#include "../../../doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/backtrackable_set_erase.hpp"
#include <set>
#include <stdexcept>

TEST_CASE("backtrackable_set_erase") {
    std::set<int> s{42};
    backtrackable_set_erase<std::set<int>> m(42);
    m.capture(s);

    SUBCASE("invoke erases value") {
        m.invoke();
        CHECK(s.count(42) == 0);

        SUBCASE("backtrack re-inserts value") {
            m.backtrack();
            CHECK(s.count(42) == 1);
        }
    }

    SUBCASE("invoke with missing value throws") {
        s.erase(42);
        CHECK_THROWS_AS(m.invoke(), std::logic_error);
    }
}
