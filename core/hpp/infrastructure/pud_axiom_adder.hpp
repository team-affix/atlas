#ifndef PUD_AXIOM_ADDER_HPP
#define PUD_AXIOM_ADDER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/rule.hpp"

template<typename IMakeAxiom,
         typename IStoreAddedUnifications,
         typename IStoreAddedBodyGoals,
         typename IStoreLvc,
         typename IStoreParent,
         typename IAllocateRootInterval,
         typename IStoreInterval,
         typename IAdoptAxiom>
struct pud_axiom_adder {
    pud_axiom_adder(IMakeAxiom& make_axiom,
                    IStoreAddedUnifications& store_added_unifications,
                    IStoreAddedBodyGoals& store_added_body_goals,
                    IStoreLvc& store_lvc,
                    IStoreParent& store_parent,
                    IAllocateRootInterval& allocate_root_interval,
                    IStoreInterval& store_interval,
                    IAdoptAxiom& adopt_axiom);
    const pud_rule_id* add_axiom(const rule& axiom);
private:
    IMakeAxiom& make_axiom_;
    IStoreAddedUnifications& store_added_unifications_;
    IStoreAddedBodyGoals& store_added_body_goals_;
    IStoreLvc& store_lvc_;
    IStoreParent& store_parent_;
    IAllocateRootInterval& allocate_root_interval_;
    IStoreInterval& store_interval_;
    IAdoptAxiom& adopt_axiom_;
    size_t next_entry_idx_;
};

template<typename IMA, typename ISAU, typename ISABG, typename ISL,
         typename ISP, typename IARI, typename ISI, typename IAO>
pud_axiom_adder<IMA, ISAU, ISABG, ISL, ISP, IARI, ISI, IAO>::pud_axiom_adder(
        IMA& make_axiom,
        ISAU& store_added_unifications,
        ISABG& store_added_body_goals,
        ISL& store_lvc,
        ISP& store_parent,
        IARI& allocate_root_interval,
        ISI& store_interval,
        IAO& adopt_axiom)
    : make_axiom_(make_axiom)
    , store_added_unifications_(store_added_unifications)
    , store_added_body_goals_(store_added_body_goals)
    , store_lvc_(store_lvc)
    , store_parent_(store_parent)
    , allocate_root_interval_(allocate_root_interval)
    , store_interval_(store_interval)
    , adopt_axiom_(adopt_axiom)
    , next_entry_idx_(0) {}

template<typename IMA, typename ISAU, typename ISABG, typename ISL,
         typename ISP, typename IARI, typename ISI, typename IAO>
const pud_rule_id* pud_axiom_adder<IMA, ISAU, ISABG, ISL, ISP, IARI, ISI, IAO>::add_axiom(
        const rule& axiom) {
    const pud_rule_id* id = make_axiom_.make_axiom(next_entry_idx_);
    ++next_entry_idx_;
    store_added_unifications_.store(id, {{0, axiom.head}});
    store_added_body_goals_.store(id, axiom.body);
    store_lvc_.store(id, axiom.var_count);
    store_parent_.store(id, nullptr);
    store_interval_.store(id, allocate_root_interval_.allocate_root());
    adopt_axiom_.adopt_axiom(id);
    return id;
}

#endif
