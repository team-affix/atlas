#ifndef PUD_AXIOM_ADDER_HPP
#define PUD_AXIOM_ADDER_HPP

#include <cstddef>
#include <utility>
#include "value_objects/om_interval.hpp"
#include "value_objects/om_label.hpp"
#include "value_objects/pud_db_node.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/rule.hpp"

template<typename IAddAxiom, typename IInstall, typename IAttachAxiom>
struct pud_axiom_adder {
    pud_axiom_adder(IAddAxiom& add_axiom,
                    IInstall& install,
                    IAttachAxiom& attach_axiom);
    const pud_rule_id* add_axiom(const rule& axiom);
private:
    IAddAxiom& add_axiom_;
    IInstall& install_;
    IAttachAxiom& attach_axiom_;
    size_t next_entry_idx_;
};

template<typename IAA, typename II, typename IAT>
pud_axiom_adder<IAA, II, IAT>::pud_axiom_adder(IAA& add_axiom,
                                              II& install,
                                              IAT& attach_axiom)
    : add_axiom_(add_axiom)
    , install_(install)
    , attach_axiom_(attach_axiom)
    , next_entry_idx_(0) {}

template<typename IAA, typename II, typename IAT>
const pud_rule_id* pud_axiom_adder<IAA, II, IAT>::add_axiom(const rule& axiom) {
    pud_db_node node{
        om_interval{om_label(nullptr), om_label(nullptr)},
        {{0, axiom.head}},
        axiom.body,
        axiom.var_count};
    const pud_rule_id* id = add_axiom_.add_axiom(next_entry_idx_, std::move(node));
    ++next_entry_idx_;
    install_.install(id);
    attach_axiom_.attach_axiom(id);
    return id;
}

#endif
