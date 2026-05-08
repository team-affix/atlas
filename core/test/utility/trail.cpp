#include "../../../doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/trail.hpp"
#include <vector>

namespace {
    struct recorder : i_backtrackable {
        std::vector<int>& order;
        int id;
        recorder(std::vector<int>& o, int id) : order(o), id(id) {}
        void backtrack() override { order.push_back(id); }
    };
}

TEST_CASE("trail") {
    trail t;

    SUBCASE("initial depth is 0") { CHECK(t.depth() == 0); }

    SUBCASE("push / pop") {
        t.push();
        SUBCASE("push increments depth")   { CHECK(t.depth() == 1); }
        SUBCASE("two pushes give depth 2") { t.push(); CHECK(t.depth() == 2); }
        SUBCASE("pop decrements depth")    { t.pop(); CHECK(t.depth() == 0); }
        SUBCASE("empty frame: pop calls nothing") {
            std::vector<int> order;
            t.pop();
            CHECK(order.empty());
            CHECK(t.depth() == 0);
        }
    }

    SUBCASE("LIFO undo order within one frame") {
        std::vector<int> order;
        t.push();
        t.log(std::make_unique<recorder>(order, 1));
        t.log(std::make_unique<recorder>(order, 2));
        t.pop();
        CHECK(order == std::vector<int>{2, 1});
    }

    SUBCASE("two frames: pop only undoes own frame") {
        std::vector<int> order;
        t.push();
        t.log(std::make_unique<recorder>(order, 1));
        t.push();
        t.log(std::make_unique<recorder>(order, 2));

        SUBCASE("first pop undoes only frame-2 entry") {
            t.pop();
            CHECK(order == std::vector<int>{2});
            CHECK(t.depth() == 1);

            SUBCASE("second pop undoes frame-1 entry") {
                order.clear();
                t.pop();
                CHECK(order == std::vector<int>{1});
                CHECK(t.depth() == 0);
            }
        }
    }
}
