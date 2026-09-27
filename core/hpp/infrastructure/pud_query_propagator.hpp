#ifndef PUD_QUERY_PROPAGATOR_HPP
#define PUD_QUERY_PROPAGATOR_HPP

#include <memory>
#include "value_objects/pud_query_node.hpp"
#include "value_objects/pud_rule_id.hpp"

template<typename Unifier, typename Normalizer, typename BindMap, typename ISpecialize>
struct pud_query_propagator {
    struct query_node_handle {
    private:
        std::shared_ptr<pud_query_node> query_node;
    };
    pud_query_propagator(
        ISpecialize& specialize,
        const pud_node& root);
    query_node_handle root();
    query_node_handle child(query_node_handle parent, const pud_rule_id* child_callee);
    query_node_handle open_query(query_node_handle caller, const expr* query);
    pud_node close_query(query_node_handle query);
private:
    ISpecialize& specialize_;
    const pud_node& root_;
};

#endif
