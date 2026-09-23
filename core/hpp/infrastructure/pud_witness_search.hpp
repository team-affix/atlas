#ifndef PUD_WITNESS_SEARCH_HPP
#define PUD_WITNESS_SEARCH_HPP

#include <cstdint>
#include <stack>
#include <vector>
#include <optional>
#include <algorithm>
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_query_id.hpp"

template<
    typename Node,
    typename IAllocateChildInterval,
    typename ISetActiveBindingInterval,
    typename IMakeVar,
    typename IUnify>
struct pud_witness_search {
    pud_witness_search(
        IAllocateChildInterval& allocate_child_interval,
        ISetActiveBindingInterval& set_active_binding_interval,
        IMakeVar& make_var,
        IUnify& unify,
        uint32_t frame_offset,
        const Node& search_root,
        om_interval search_root_interval);
    bool resume();
private:
    struct frame {
        const pud_rule_id* callee;
        om_interval        interval;
    };

    bool enter_any();
    bool try_enter(const Node& node, om_interval interval);

    IAllocateChildInterval& allocate_child_interval_;
    ISetActiveBindingInterval& set_active_binding_interval_;
    IMakeVar& make_var_;
    IUnify& unify_;

    const Node* current_;
    om_interval current_interval_;
    uint32_t frame_offset_;
    std::stack<frame> frame_stack_;
};

template<typename N, typename IACI, typename ISABI, typename IMV, typename IU>
pud_witness_search<N, IACI, ISABI, IMV, IU>::pud_witness_search(
    IACI& allocate_child_interval,
    ISABI& set_active_binding_interval,
    IMV& make_var,
    IU& unify,
    uint32_t frame_offset,
    const N& search_root,
    om_interval search_root_interval)
    : allocate_child_interval_(allocate_child_interval),
    set_active_binding_interval_(set_active_binding_interval),
    make_var_(make_var),
    unify_(unify),
    current_(&search_root),
    current_interval_(search_root_interval),
    frame_offset_(frame_offset) {}

template<typename N, typename IACI, typename ISABI, typename IMV, typename IU>
bool pud_witness_search<N, IACI, ISABI, IMV, IU>::resume() {
    do {
        const N& node = *current_;

        if (!node.children.has_value())
            return true; // leaf reached
        
        for (const auto& child : node.children.value())
            frame_stack_.push(frame{child, allocate_child_interval_.allocate_child_of(current_interval_)});

    } while(enter_any());

    return false;
}

template<typename N, typename IACI, typename ISABI, typename IMV, typename IU>
bool pud_witness_search<N, IACI, ISABI, IMV, IU>::enter_any() {
    while (!frame_stack_.empty()) {
        frame top_frame = frame_stack_.top();
        frame_stack_.pop();
        if (try_enter(*top_frame.callee, top_frame.interval))
            return true;
    }
    return false;
}


template<typename N, typename IACI, typename ISABI, typename IMV, typename IU>
bool pud_witness_search<N, IACI, ISABI, IMV, IU>::try_enter(const N& node, om_interval interval) {
    set_active_binding_interval_.set(interval);
    bool result = std::all_of(
        node.added_unifications.begin(),
        node.added_unifications.end(),
        [this](const auto& added_unification) {
            framed_expr lhs{make_var_.make_var(added_unification.var_idx), frame_offset_};
            framed_expr rhs{added_unification.value, frame_offset_};
            return unify_.unify(lhs, rhs);
        });
    if (!result)
        return false;
    current_ = &node;
    current_interval_ = interval;
    return true;
}

#endif
