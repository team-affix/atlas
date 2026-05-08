#include "../../../external/doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/backtrackable_set_insert.hpp"
#include <set>

TEST_CASE("backtrackable_set_insert") {
    std::set<int> s;
    backtrackable_set_insert<std::set<int>> m(9);
    m.capture(s);

    SUBCASE("invoke inserts value") {
        m.invoke();
        CHECK(s.count(9) == 1);

        SUBCASE("backtrack removes value") {
            m.backtrack();
            CHECK(s.count(9) == 0);
        }
    }
}
