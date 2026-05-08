#include "../../../doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/backtrackable_set_insert.hpp"
#include <set>
#include <stdexcept>

TEST_CASE("backtrackable_set_insert") {
    std::set<int> s;
    backtrackable_set_insert<std::set<int>> m(7);
    m.capture(s);

    SUBCASE("invoke inserts value") {
        m.invoke();
        CHECK(s.count(7) == 1);

        SUBCASE("backtrack removes value") {
            m.backtrack();
            CHECK(s.count(7) == 0);
        }
    }

    SUBCASE("invoke with duplicate value throws") {
        s.insert(7);
        CHECK_THROWS_AS(m.invoke(), std::logic_error);
    }
}
