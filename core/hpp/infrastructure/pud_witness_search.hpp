#ifndef PUD_WITNESS_SEARCH_HPP
#define PUD_WITNESS_SEARCH_HPP

#include <cstdint>
#include <stack>
#include "value_objects/om_interval.hpp"

template<
    typename Node,
    typename IAllocateChildInterval,
    typename IEnterNode>
struct pud_witness_search {
    pud_witness_search(
        IAllocateChildInterval& allocate_child_interval,
        IEnterNode& enter_node,
        uint32_t frame_offset,
        const Node& search_root,
        om_interval search_root_interval);
    bool resume();
private:
    struct frame {
        const Node& node;
        om_interval interval;
    };

    bool enter_any();

    IAllocateChildInterval& allocate_child_interval_;
    IEnterNode& enter_node_;

    uint32_t frame_offset_;
    const Node* current_;
    om_interval current_interval_;
    std::stack<frame> frame_stack_;
};

template<typename N, typename IACI, typename IEN>
pud_witness_search<N, IACI, IEN>::pud_witness_search(
    IACI& allocate_child_interval,
    IEN& enter_node,
    uint32_t frame_offset,
    const N& search_root,
    om_interval search_root_interval)
    : allocate_child_interval_(allocate_child_interval),
    enter_node_(enter_node),
    current_(&search_root),
    current_interval_(search_root_interval),
    frame_offset_(frame_offset) {}

template<typename N, typename IACI, typename IEN>
bool pud_witness_search<N, IACI, IEN>::resume() {
    do {
        const N& node = *current_;

        if (!node.children.has_value())
            return true; // leaf reached
        
        for (const auto& child : node.children.value())
            frame_stack_.push(frame{child, allocate_child_interval_.allocate_child_of(current_interval_)});

    } while(enter_any());

    return false;
}

template<typename N, typename IACI, typename IEN>
bool pud_witness_search<N, IACI, IEN>::enter_any() {
    while (!frame_stack_.empty()) {
        frame top_frame = frame_stack_.top();
        frame_stack_.pop();
        if (enter_node_.try_enter(top_frame.node, top_frame.interval, frame_offset_)) {
            current_ = &top_frame.node;
            current_interval_ = top_frame.interval;
            return true;
        }
    }
    return false;
}

#endif
