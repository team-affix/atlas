#include "../../../external/doctest/doctest/doctest.h"
#include "../../../core/hpp/utility/sequencer.hpp"
#include "../../../core/hpp/utility/trail.hpp"

TEST_CASE("sequencer<int>") {
    trail t;
    sequencer<int> seq(t);

    SUBCASE("next returns 0, 1, 2 in order") {
        CHECK(seq.next() == 0);
        CHECK(seq.next() == 1);
        CHECK(seq.next() == 2);
    }

    SUBCASE("pop reverts counter") {
        seq.next(); seq.next();  // advance to 2
        t.push();
        seq.next(); seq.next(); seq.next();  // advance to 5 inside frame
        t.pop();
        CHECK(seq.next() == 2);  // resumed from before push
    }

    SUBCASE("two sequencers sharing a trail revert independently") {
        sequencer<int> seq2(t);
        seq.next(); seq.next();   // seq at 2
        seq2.next();              // seq2 at 1
        t.push();
        seq.next();               // seq at 3
        seq2.next(); seq2.next(); // seq2 at 3
        t.pop();
        CHECK(seq.next() == 2);
        CHECK(seq2.next() == 1);
    }
}
