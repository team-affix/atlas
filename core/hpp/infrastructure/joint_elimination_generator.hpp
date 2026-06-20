#ifndef JOINT_ELIMINATION_GENERATOR_HPP
#define JOINT_ELIMINATION_GENERATOR_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"

template<typename ICdcl, typename IMhu>
struct joint_elimination_generator {
    joint_elimination_generator(ICdcl&, IMhu&);
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
private:
    ICdcl& cdcl;
    IMhu& mhu;
};

template<typename ICdcl, typename IMhu>
joint_elimination_generator<ICdcl, IMhu>::joint_elimination_generator(ICdcl& c, IMhu& m)
    : cdcl(c), mhu(m) {}

template<typename ICdcl, typename IMhu>
coroutine<const resolution_lineage*, void>
joint_elimination_generator<ICdcl, IMhu>::constrain(const resolution_lineage* rl) {
    auto cdcl_sm = cdcl.constrain(rl);
    while (!cdcl_sm.done()) {
        cdcl_sm.resume();
        if (cdcl_sm.has_yield())
            co_yield cdcl_sm.consume_yield();
    }
    auto mhu_sm = mhu.constrain(rl);
    while (!mhu_sm.done()) {
        mhu_sm.resume();
        if (mhu_sm.has_yield())
            co_yield mhu_sm.consume_yield();
    }
    co_return;
}

#endif
