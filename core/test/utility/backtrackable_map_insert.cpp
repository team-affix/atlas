#include "../../../external/doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/backtrackable_map_insert.hpp"
#include <map>

TEST_CASE("backtrackable_map_insert") {
    std::map<int, int> mp;
    backtrackable_map_insert<std::map<int, int>> m(7, 42);
    m.capture(mp);

    SUBCASE("invoke inserts entry") {
        m.invoke();
        CHECK(mp.count(7) == 1);
        CHECK(mp.at(7) == 42);

        SUBCASE("backtrack removes entry") {
            m.backtrack();
            CHECK(mp.count(7) == 0);
        }
    }
}
