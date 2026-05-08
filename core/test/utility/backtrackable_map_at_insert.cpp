#include "../../../external/doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/backtrackable_map_at_insert.hpp"
#include <map>
#include <set>

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
}
