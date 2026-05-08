#include "../../../external/doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/backtrackable_map_at_erase.hpp"
#include <map>
#include <set>

TEST_CASE("backtrackable_map_at_erase") {
    std::map<int, std::set<int>> mp{{1, {55}}};
    backtrackable_map_at_erase<std::map<int, std::set<int>>> m(1, 55);
    m.capture(mp);

    SUBCASE("invoke erases from inner set") {
        m.invoke();
        CHECK(mp.at(1).count(55) == 0);

        SUBCASE("backtrack re-inserts into inner set") {
            m.backtrack();
            CHECK(mp.at(1).count(55) == 1);
        }
    }
}
