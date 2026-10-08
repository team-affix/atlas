#ifndef PUD_WITNESS_SEARCH_HEAD_HPP
#define PUD_WITNESS_SEARCH_HEAD_HPP

#include <deque>
#include <optional>
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_witness_advance_result.hpp"
#include "debug_assert.hpp"

template<
    typename QueryHandle,
    typename NodeIterator,
    typename ICheckNodeLeaf,
    typename IGetNodeChildren,
    typename IDescendQueryNodeHandle,
    typename IGetCallSiteIdx>
struct pud_witness_search_head {
    pud_witness_search_head(
        ICheckNodeLeaf& check_node_leaf,
        IGetNodeChildren& get_node_children,
        IDescendQueryNodeHandle& descend_query_node_handle,
        IGetCallSiteIdx& get_call_site_idx,
        QueryHandle search_root_handle);
    pud_witness_search_head(
        const pud_witness_search_head& other,
        QueryHandle search_root_handle);
    std::optional<pud_witness_advance_result<QueryHandle, NodeIterator>> advance();
    std::optional<pud_node_id> resume();
private:
    struct frame {
        std::optional<QueryHandle> handle;
        QueryHandle parent_handle;
        NodeIterator next_sibling_it;
        NodeIterator end_sibling_it;
    };

    std::optional<QueryHandle> try_replace_sibling(const QueryHandle& parent_handle, NodeIterator& next_sibling_it, NodeIterator end_sibling_it);
    void descend(QueryHandle parent_handle, NodeIterator next_sibling_it, NodeIterator end_sibling_it);
    void ascend();

    ICheckNodeLeaf& check_node_leaf_;
    IGetNodeChildren& get_node_children_;
    IDescendQueryNodeHandle& descend_query_node_handle_;
    IGetCallSiteIdx& get_call_site_idx_;

    QueryHandle search_root_handle_;
    std::deque<frame> frame_stack_;
};

template<typename QH, typename NI, typename ICNL, typename IGCN, typename IDQN, typename IGCSI>
pud_witness_search_head<QH, NI, ICNL, IGCN, IDQN, IGCSI>::pud_witness_search_head(
    ICNL& check_node_leaf,
    IGCN& get_node_children,
    IDQN& descend_query_node_handle,
    IGCSI& get_call_site_idx,
    QH search_root_handle) :
    check_node_leaf_(check_node_leaf),
    get_node_children_(get_node_children),
    descend_query_node_handle_(descend_query_node_handle),
    get_call_site_idx_(get_call_site_idx),
    search_root_handle_(search_root_handle)
{}

template<typename QH, typename CI, typename ICNL, typename IGCN, typename IDQN, typename IGCSI>
pud_witness_search_head<QH, CI, ICNL, IGCN, IDQN, IGCSI>::pud_witness_search_head(
    const pud_witness_search_head& other,
    QH search_root_handle) :
    check_node_leaf_(other.check_node_leaf_),
    get_node_children_(other.get_node_children_),
    descend_query_node_handle_(other.descend_query_node_handle_),
    get_call_site_idx_(other.get_call_site_idx_),
    search_root_handle_(search_root_handle)
{
    // walk our frame stack using the new query handle
    // if we cannot proceed, don't search. just leave the current stack

    QH parent_handle = search_root_handle;
    
    for (const auto& other_frame : other.frame_stack_) {
        
        DEBUG_ASSERT(other_frame.handle.has_value());
        
        const QH& other_handle = other_frame.handle.value();

        frame new_frame = {
            .handle = descend_query_node_handle_.descend(
                parent_handle, other_handle.node()),
            .parent_handle = parent_handle,
            .next_sibling_it = other_frame.next_sibling_it,
            .end_sibling_it = other_frame.end_sibling_it,
        };
        
        frame_stack_.push_back(new_frame);

        if (!new_frame.handle.has_value())
            break; // handle will be null and will try to be replaced by siblings
    
        parent_handle = new_frame.handle.value();
    }
}

template<typename QH, typename NI, typename ICNL, typename IGCN, typename IDQN, typename IGCSI>
std::optional<pud_witness_advance_result<QH, NI>> pud_witness_search_head<QH, NI, ICNL, IGCN, IDQN, IGCSI>::advance() {
    if (frame_stack_.empty())
        return std::nullopt;

    frame first_stack_frame = frame_stack_.front();

    frame_stack_.pop_front();

    DEBUG_ASSERT(first_stack_frame.handle.has_value());

    pud_witness_advance_result<QH, NI> result = {
        .root_handle = first_stack_frame.handle.value(),
        .root_next_sibling_it = first_stack_frame.next_sibling_it,
        .root_end_sibling_it = first_stack_frame.end_sibling_it,
    };

    search_root_handle_ = first_stack_frame.handle.value();

    return result;
}

template<typename QH, typename NI, typename ICNL, typename IGCN, typename IDQN, typename IGCSI>
std::optional<pud_node_id> pud_witness_search_head<QH, NI, ICNL, IGCN, IDQN, IGCSI>::resume() {
    // handle empty stack
    if (frame_stack_.empty()) {
        if (check_node_leaf_.check_leaf(search_root_handle_.node()))
            return search_root_handle_.node();

        const auto& children = get_node_children_.get(search_root_handle_.node());

        descend(search_root_handle_, children.begin(), children.end());
    }
    
    // basic idea:
    //     at any point in time, if the current frame is a leaf, we are done.
    //     while not leaf, get children. initialize the iterator to begin of the children.
    
    while (!frame_stack_.empty()) {
        // get children of current frame
        frame& current_frame = frame_stack_.back();

        const auto& optional_current_handle = current_frame.handle;

        if (!optional_current_handle.has_value()) {
            // if we are backtracking OR need to initialize handle,
            // its the same algo.

            const QH& parent_handle = current_frame.parent_handle;

            // try to propagate the query to the next sibling
            current_frame.handle = try_replace_sibling(
                parent_handle, current_frame.next_sibling_it, current_frame.end_sibling_it);

            if (!current_frame.handle.has_value()) {
                ascend();
                continue;
            }
        }

        // handle leaf check / children initialization

        const QH& current_handle = optional_current_handle.value();
        
        pud_node_id current_node = current_handle.node();
        
        if (check_node_leaf_.check_leaf(current_node))
            return current_node;

        const auto& children = get_node_children_.get(current_node);
        
        descend(current_handle, children.begin(), children.end());
    }

    return std::nullopt;
}

template<typename QH, typename NI, typename ICNL, typename IGCN, typename IDQN, typename IGCSI>
std::optional<QH> pud_witness_search_head<QH, NI, ICNL, IGCN, IDQN, IGCSI>::try_replace_sibling(const QH& parent_handle, NI& next_sibling_it, NI end_sibling_it) {
    while (next_sibling_it != end_sibling_it) {
        if (auto next_sibling_handle =
            descend_query_node_handle_.descend(
                parent_handle, *(next_sibling_it++))) {
            return next_sibling_handle;
        }
    }
    return std::nullopt;
}

template<typename QH, typename NI, typename ICNL, typename IGCN, typename IDQN, typename IGCSI>
void pud_witness_search_head<QH, NI, ICNL, IGCN, IDQN, IGCSI>::descend(QH parent_handle, NI next_sibling_it, NI end_sibling_it) {
    frame_stack_.push_back({
        .handle = std::nullopt,
        .parent_handle = parent_handle,
        .next_sibling_it = next_sibling_it,
        .end_sibling_it = end_sibling_it
    });
}

template<typename QH, typename NI, typename ICNL, typename IGCN, typename IDQN, typename IGCSI>
void pud_witness_search_head<QH, NI, ICNL, IGCN, IDQN, IGCSI>::ascend() {
    frame_stack_.pop_back();
    if (!frame_stack_.empty())
        frame_stack_.back().handle = std::nullopt;
}

#endif
