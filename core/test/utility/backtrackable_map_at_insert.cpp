#include "../../../doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/backtrackable_map_at_insert.hpp"
#include <map>
#include <set>
#include <stdexcept>

TEST_CASE("backtrackable_map_at_insert") {
    std::map<int, std::set<int>> mp{{1, {}}};
    backtrackable_map_at_insert<std::map<int, std::set<int>>> m(1, 55);
    m.capture(mp);

    SUBCASE("invoke inserts into inner set") {
        m.invoke();
        CHECK(mp.at(1).count(55) == 1);

        SUBCASE("backtrack removes from inner set") {
            m.backtrack();
            CHECK(mp.at(1).count(55) == 0);
        }
    }

    SUBCASE("invoke with missing outer key throws") {
        backtrackable_map_at_insert<std::map<int, std::set<int>>> bad(42, 55);
        bad.capture(mp);
        CHECK_THROWS_AS(bad.invoke(), std::out_of_range);
    }

    SUBCASE("invoke with duplicate inner value throws") {
        mp.at(1).insert(55);
        CHECK_THROWS_AS(m.invoke(), std::logic_error);
    }
}
