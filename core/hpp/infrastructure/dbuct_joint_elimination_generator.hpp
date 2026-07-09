#ifndef DBUCT_JOINT_ELIMINATION_GENERATOR_HPP
#define DBUCT_JOINT_ELIMINATION_GENERATOR_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"

// dbuct-specific joint eliminator. The generic joint_elimination_generator is
// shared by 5 non-dbuct manifests and must not change; dbuct needs an extra
// obligation: every CDCL-forced elimination must also drop that lineage's head
// from the MHU unit (the cdcl/mhu interaction is now coupled -- a goal CDCL forces
// out can no longer be a live MHU head). MHU already self-removes the heads it
// forces via its own constrain, so we only remove_head on the CDCL yields.
template<typename ICdcl, typename IMhu>
struct dbuct_joint_elimination_generator {
    dbuct_joint_elimination_generator(ICdcl&, IMhu&);
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
private:
    ICdcl& cdcl;
    IMhu& mhu;
};

template<typename ICdcl, typename IMhu>
dbuct_joint_elimination_generator<ICdcl, IMhu>::dbuct_joint_elimination_generator(ICdcl& c, IMhu& m)
    : cdcl(c), mhu(m) {}

template<typename ICdcl, typename IMhu>
coroutine<const resolution_lineage*, void>
dbuct_joint_elimination_generator<ICdcl, IMhu>::constrain(const resolution_lineage* rl) {
    auto cdcl_sm = cdcl.constrain(rl);
    while (!cdcl_sm.done()) {
        cdcl_sm.resume();
        if (cdcl_sm.has_yield()) {
            const resolution_lineage* elim = cdcl_sm.consume_yield();
            mhu.remove_head(elim);
            co_yield elim;
        }
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
