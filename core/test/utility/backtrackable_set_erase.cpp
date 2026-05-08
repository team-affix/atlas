#include "../../../external/doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/backtrackable_set_erase.hpp"
#include <set>

TEST_CASE("backtrackable_set_erase") {
    std::set<int> s{9};
    backtrackable_set_erase<std::set<int>> m(9);
    m.capture(s);

    SUBCASE("invoke removes value") {
        m.invoke();
        CHECK(s.count(9) == 0);

        SUBCASE("backtrack re-inserts value") {
            m.backtrack();
            CHECK(s.count(9) == 1);
        }
    }
}
