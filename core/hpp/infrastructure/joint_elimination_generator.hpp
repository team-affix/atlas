#ifndef JOINT_ELIMINATION_GENERATOR_HPP
#define JOINT_ELIMINATION_GENERATOR_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"

template<typename IConstrainCdcl, typename IConstrainMhu>
struct joint_elimination_generator {
    joint_elimination_generator(IConstrainCdcl&, IConstrainMhu&);
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
private:
    IConstrainCdcl& constrain_cdcl_;
    IConstrainMhu& constrain_mhu_;
};

template<typename ICC, typename ICM>
joint_elimination_generator<ICC, ICM>::joint_elimination_generator(ICC& c, ICM& m)
    : constrain_cdcl_(c), constrain_mhu_(m) {}

template<typename ICC, typename ICM>
coroutine<const resolution_lineage*, void>
joint_elimination_generator<ICC, ICM>::constrain(const resolution_lineage* rl) {
    auto cdcl_sm = constrain_cdcl_.constrain(rl);
    while (!cdcl_sm.done()) {
        cdcl_sm.resume();
        if (cdcl_sm.has_yield())
            co_yield cdcl_sm.consume_yield();
    }
    auto mhu_sm = constrain_mhu_.constrain(rl);
    while (!mhu_sm.done()) {
        mhu_sm.resume();
        if (mhu_sm.has_yield())
            co_yield mhu_sm.consume_yield();
    }
    co_return;
}

#endif
