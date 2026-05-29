#include "infrastructure/joint_elimination_generator.hpp"
#include "infrastructure/cdcl_elimination_generator.hpp"
#include "infrastructure/mhu_elimination_generator.hpp"

joint_elimination_generator::joint_elimination_generator(locator& loc)
    : cdcl(static_cast<i_elimination_generator&>(loc.locate<cdcl_elimination_generator>())),
      mhu(static_cast<i_elimination_generator&>(loc.locate<mhu_elimination_generator>())) {
}

coroutine<const resolution_lineage*, void> joint_elimination_generator::constrain(const resolution_lineage* rl) {
    // 1. constrain cdcl
    auto cdcl_sm = cdcl.constrain(rl);
    while (!cdcl_sm.done()) {
        cdcl_sm.resume();
        if (cdcl_sm.has_yield())
            co_yield cdcl_sm.consume_yield();
    }
    // 2. constrain mhu
    auto mhu_sm = mhu.constrain(rl);
    while (!mhu_sm.done()) {
        mhu_sm.resume();
        if (mhu_sm.has_yield())
            co_yield mhu_sm.consume_yield();
    }
    co_return;
}
