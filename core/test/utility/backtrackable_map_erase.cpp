#include "../../../external/doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/backtrackable_map_erase.hpp"
#include <map>

TEST_CASE("backtrackable_map_erase") {
    std::map<int, int> mp{{3, 77}};
    backtrackable_map_erase<std::map<int, int>> m(3);
    m.capture(mp);

    SUBCASE("invoke captures value and erases entry") {
        m.invoke();
        CHECK(mp.count(3) == 0);

        SUBCASE("backtrack re-inserts with correct value") {
            m.backtrack();
            CHECK(mp.count(3) == 1);
            CHECK(mp.at(3) == 77);
        }
    }
}
