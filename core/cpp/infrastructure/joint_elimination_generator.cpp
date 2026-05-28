#include "infrastructure/joint_elimination_generator.hpp"

joint_elimination_generator::joint_elimination_generator(
    i_elimination_generator& cdcl,
    i_elimination_generator& mhu)
    : cdcl(cdcl), mhu(mhu) {
}

state_machine<const resolution_lineage*> joint_elimination_generator::constrain(const resolution_lineage* rl) {
    // 1. constrain cdcl
    auto cdcl_sm = cdcl.constrain(rl);
    while (!cdcl_sm.done()) {
        auto res = cdcl_sm.resume();
        if (res.has_value())
            co_yield res.value();
    }
    // 2. constrain mhu
    auto mhu_sm = mhu.constrain(rl);
    while (!mhu_sm.done()) {
        auto res = mhu_sm.resume();
        if (res.has_value())
            co_yield res.value();
    }
    // 3. done
    co_yield nullptr;
}
