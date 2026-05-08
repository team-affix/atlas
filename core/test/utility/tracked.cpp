#include "../../../doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/tracked.hpp"
#include "../../../core/hpp/utility/trail.hpp"
#include "../../../core/hpp/utility/backtrackable_increment.hpp"

TEST_CASE("tracked<int>") {
    trail t;
    tracked<int> v(t, 10);

    SUBCASE("get returns initial value") { CHECK(v.get() == 10); }

    SUBCASE("mutate changes value") {
        t.push();
        v.mutate(std::make_unique<backtrackable_increment<int>>());
        CHECK(v.get() == 11);

        SUBCASE("pop reverts to pre-push value") {
            t.pop();
            CHECK(v.get() == 10);
        }

        SUBCASE("two mutations in one frame both revert") {
            v.mutate(std::make_unique<backtrackable_increment<int>>());
            CHECK(v.get() == 12);
            t.pop();
            CHECK(v.get() == 10);
        }
    }

    SUBCASE("two independent frames revert independently") {
        t.push();
        v.mutate(std::make_unique<backtrackable_increment<int>>());  // v = 11
        t.push();
        v.mutate(std::make_unique<backtrackable_increment<int>>());  // v = 12

        SUBCASE("inner pop reverts to 11") {
            t.pop();
            CHECK(v.get() == 11);
        }
        SUBCASE("both pops revert to 10") {
            t.pop();
            t.pop();
            CHECK(v.get() == 10);
        }
    }
}
