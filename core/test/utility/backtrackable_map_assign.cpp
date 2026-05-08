#include "../../../external/doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/backtrackable_map_assign.hpp"
#include <map>

TEST_CASE("backtrackable_map_assign") {
    std::map<int, int> mp{{1, 10}};
    backtrackable_map_assign<std::map<int, int>> m(1, 99);
    m.capture(mp);

    SUBCASE("invoke sets new value") {
        m.invoke();
        CHECK(mp.at(1) == 99);

        SUBCASE("backtrack restores old value") {
            m.backtrack();
            CHECK(mp.at(1) == 10);
        }
    }
}
