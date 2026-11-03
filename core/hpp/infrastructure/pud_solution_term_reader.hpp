#ifndef PUD_SOLUTION_TERM_READER_HPP
#define PUD_SOLUTION_TERM_READER_HPP

#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_solution_term.hpp"
#include "value_objects/rule.hpp"

template<typename IGetNodeAxiom, typename IGetAxiom, typename IGetNodeBindings>
struct pud_solution_term_reader {
    pud_solution_term_reader(
        IGetNodeAxiom& get_node_axiom, IGetAxiom& get_axiom, IGetNodeBindings& get_node_bindings);
    pud_solution_term read(pud_global_node_id leaf) const;
private:
    IGetNodeAxiom& get_node_axiom_;
    IGetAxiom& get_axiom_;
    IGetNodeBindings& get_node_bindings_;
};

template<typename IGetNodeAxiom, typename IGetAxiom, typename IGetNodeBindings>
pud_solution_term_reader<IGetNodeAxiom, IGetAxiom, IGetNodeBindings>::pud_solution_term_reader(
    IGetNodeAxiom& get_node_axiom, IGetAxiom& get_axiom, IGetNodeBindings& get_node_bindings)
    : get_node_axiom_(get_node_axiom)
    , get_axiom_(get_axiom)
    , get_node_bindings_(get_node_bindings) {}

template<typename IGetNodeAxiom, typename IGetAxiom, typename IGetNodeBindings>
pud_solution_term pud_solution_term_reader<IGetNodeAxiom, IGetAxiom, IGetNodeBindings>::read(
    pud_global_node_id leaf) const {
    const pud_global_node_id axiom = get_node_axiom_.get(leaf);
    return pud_solution_term{
        .head = get_axiom_.get_axiom(axiom).head,
        .module = axiom.module,
        .bindings = get_node_bindings_.get(leaf),
    };
}

#endif
