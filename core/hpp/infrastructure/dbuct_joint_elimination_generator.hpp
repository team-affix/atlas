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
template<typename IConstrainCdcl, typename IConstrainMhu>
struct dbuct_joint_elimination_generator {
    dbuct_joint_elimination_generator(IConstrainCdcl&, IConstrainMhu&);
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
private:
    IConstrainCdcl& constrain_cdcl_;
    IConstrainMhu& constrain_mhu_;
};

template<typename ICC, typename ICM>
dbuct_joint_elimination_generator<ICC, ICM>::dbuct_joint_elimination_generator(ICC& c, ICM& m)
    : constrain_cdcl_(c), constrain_mhu_(m) {}

template<typename ICC, typename ICM>
coroutine<const resolution_lineage*, void>
dbuct_joint_elimination_generator<ICC, ICM>::constrain(const resolution_lineage* rl) {
    auto cdcl_sm = constrain_cdcl_.constrain(rl);
    while (!cdcl_sm.done()) {
        cdcl_sm.resume();
        if (cdcl_sm.has_yield()) {
            const resolution_lineage* elim = cdcl_sm.consume_yield();
            constrain_mhu_.remove_head(elim);
            co_yield elim;
        }
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
