#ifndef PUD_NODE_ENTERER_HPP
#define PUD_NODE_ENTERER_HPP

#include <algorithm>
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/framed_expr.hpp"

template<
    typename Node,
    typename ISetActiveBindingInterval,
    typename IUnify,
    typename IMakeVar>
struct pud_node_enterer {
    pud_node_enterer(
        ISetActiveBindingInterval& set_active_binding_interval,
        IUnify& unify,
        IMakeVar& make_var);
    bool enter(const Node& node, om_interval interval, uint32_t frame_offset);
private:
    ISetActiveBindingInterval& set_active_binding_interval_;
    IUnify& unify_;
    IMakeVar& make_var_;
};

template<typename N, typename ISABI, typename IU, typename IMV>
pud_node_enterer<N, ISABI, IU, IMV>::pud_node_enterer(
    ISABI& set_active_binding_interval,
    IU& unify,
    IMV& make_var)
    : set_active_binding_interval_(set_active_binding_interval),
    unify_(unify),
    make_var_(make_var) {
}

template<typename N, typename ISABI, typename IU, typename IMV>
bool pud_node_enterer<N, ISABI, IU, IMV>::enter(const N& node, om_interval interval, uint32_t frame_offset) {
    set_active_binding_interval_.set(interval);
    return std::all_of(
        node.added_unifications.begin(),
        node.added_unifications.end(),
        [this, frame_offset](const auto& added_unification) {
            framed_expr lhs{make_var_.make_var(added_unification.var_idx), frame_offset};
            framed_expr rhs{added_unification.value, frame_offset};
            return unify_.unify(lhs, rhs);
        });
}

#endif
