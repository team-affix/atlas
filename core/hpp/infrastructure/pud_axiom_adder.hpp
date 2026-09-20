#ifndef PUD_AXIOM_ADDER_HPP
#define PUD_AXIOM_ADDER_HPP

#include <cstddef>
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/rule.hpp"

template<typename IAddAxiom, typename IAdoptAxiom>
struct pud_axiom_adder {
    pud_axiom_adder(IAddAxiom& add_axiom, IAdoptAxiom& adopt_axiom);
    const pud_rule_id* add_axiom(const rule& axiom);
private:
    IAddAxiom& add_axiom_;
    IAdoptAxiom& adopt_axiom_;
    size_t next_entry_idx_;
};

template<typename IAA, typename IAO>
pud_axiom_adder<IAA, IAO>::pud_axiom_adder(IAA& add_axiom, IAO& adopt_axiom)
    : add_axiom_(add_axiom)
    , adopt_axiom_(adopt_axiom)
    , next_entry_idx_(0) {}

template<typename IAA, typename IAO>
const pud_rule_id* pud_axiom_adder<IAA, IAO>::add_axiom(const rule& axiom) {
    const pud_rule_id* id = add_axiom_.add_axiom(
        next_entry_idx_,
        {{0, axiom.head}},
        axiom.body,
        axiom.var_count);
    ++next_entry_idx_;
    adopt_axiom_.adopt_axiom(id);
    return id;
}

#endif
