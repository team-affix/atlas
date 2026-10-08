#ifndef PUD_WITNESS_SEARCH_HEAD_HPP
#define PUD_WITNESS_SEARCH_HEAD_HPP

#include <deque>
#include <optional>
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_witness_advance_result.hpp"
#include "debug_assert.hpp"

template<
    typename Descent,
    typename NodeIterator,
    typename ICheckNodeLeaf,
    typename IGetNodeChildren,
    typename IDescend,
    typename IGetCallSiteIdx>
struct pud_witness_search_head {
    pud_witness_search_head(
        ICheckNodeLeaf& check_node_leaf,
        IGetNodeChildren& get_node_children,
        IDescend& descend,
        IGetCallSiteIdx& get_call_site_idx,
        Descent search_root_descent);
    pud_witness_search_head(
        const pud_witness_search_head& other,
        Descent search_root_descent);
    std::optional<pud_witness_advance_result<Descent, NodeIterator>> advance();
    std::optional<pud_node_id> resume();
private:
    struct frame {
        std::optional<Descent> descent;
        Descent parent_descent;
        NodeIterator next_sibling_it;
        NodeIterator end_sibling_it;
    };

    std::optional<Descent> try_replace_sibling(const Descent& parent_descent, NodeIterator& next_sibling_it, NodeIterator end_sibling_it);
    void descend(Descent parent_descent, NodeIterator next_sibling_it, NodeIterator end_sibling_it);
    void ascend();

    ICheckNodeLeaf& check_node_leaf_;
    IGetNodeChildren& get_node_children_;
    IDescend& descend_;
    IGetCallSiteIdx& get_call_site_idx_;

    Descent search_root_descent_;
    std::deque<frame> frame_stack_;
};

template<typename D, typename NI, typename ICNL, typename IGCN, typename IDESC, typename IGCSI>
pud_witness_search_head<D, NI, ICNL, IGCN, IDESC, IGCSI>::pud_witness_search_head(
    ICNL& check_node_leaf,
    IGCN& get_node_children,
    IDESC& descend,
    IGCSI& get_call_site_idx,
    D search_root_descent) :
    check_node_leaf_(check_node_leaf),
    get_node_children_(get_node_children),
    descend_(descend),
    get_call_site_idx_(get_call_site_idx),
    search_root_descent_(search_root_descent)
{}

template<typename D, typename CI, typename ICNL, typename IGCN, typename IDESC, typename IGCSI>
pud_witness_search_head<D, CI, ICNL, IGCN, IDESC, IGCSI>::pud_witness_search_head(
    const pud_witness_search_head& other,
    D search_root_descent) :
    check_node_leaf_(other.check_node_leaf_),
    get_node_children_(other.get_node_children_),
    descend_(other.descend_),
    get_call_site_idx_(other.get_call_site_idx_),
    search_root_descent_(search_root_descent)
{
    // walk our frame stack using the new root descent
    // if we cannot proceed, don't search. just leave the current stack

    D parent_descent = search_root_descent;
    
    for (const auto& other_frame : other.frame_stack_) {
        
        DEBUG_ASSERT(other_frame.descent.has_value());
        
        const D& other_descent = other_frame.descent.value();

        frame new_frame = {
            .descent = descend_.descend(
                parent_descent, other_descent.node),
            .parent_descent = parent_descent,
            .next_sibling_it = other_frame.next_sibling_it,
            .end_sibling_it = other_frame.end_sibling_it,
        };
        
        frame_stack_.push_back(new_frame);

        if (!new_frame.descent.has_value())
            break; // handle will be null and will try to be replaced by siblings
    
        parent_descent = new_frame.descent.value();
    }
}

template<typename D, typename NI, typename ICNL, typename IGCN, typename IDESC, typename IGCSI>
std::optional<pud_witness_advance_result<D, NI>> pud_witness_search_head<D, NI, ICNL, IGCN, IDESC, IGCSI>::advance() {
    if (frame_stack_.empty())
        return std::nullopt;

    frame first_stack_frame = frame_stack_.front();

    frame_stack_.pop_front();

    DEBUG_ASSERT(first_stack_frame.descent.has_value());

    pud_witness_advance_result<D, NI> result = {
        .root_descent = first_stack_frame.descent.value(),
        .root_next_sibling_it = first_stack_frame.next_sibling_it,
        .root_end_sibling_it = first_stack_frame.end_sibling_it,
    };

    search_root_descent_ = first_stack_frame.descent.value();

    return result;
}

template<typename D, typename NI, typename ICNL, typename IGCN, typename IDESC, typename IGCSI>
std::optional<pud_node_id> pud_witness_search_head<D, NI, ICNL, IGCN, IDESC, IGCSI>::resume() {
    // handle empty stack
    if (frame_stack_.empty()) {
        if (check_node_leaf_.check_leaf(search_root_descent_.node))
            return search_root_descent_.node;

        const auto& children = get_node_children_.get(search_root_descent_.node);

        descend(search_root_descent_, children.begin(), children.end());
    }
    
    // basic idea:
    //     at any point in time, if the current frame is a leaf, we are done.
    //     while not leaf, get children. initialize the iterator to begin of the children.
    
    while (!frame_stack_.empty()) {
        // get children of current frame
        frame& current_frame = frame_stack_.back();

        const auto& optional_current_descent = current_frame.descent;

        if (!optional_current_descent.has_value()) {
            // if we are backtracking OR need to initialize handle,
            // its the same algo.

            const D& parent_descent = current_frame.parent_descent;

            // try to propagate the query to the next sibling
            current_frame.descent = try_replace_sibling(
                parent_descent, current_frame.next_sibling_it, current_frame.end_sibling_it);

            if (!current_frame.descent.has_value()) {
                ascend();
                continue;
            }
        }

        // handle leaf check / children initialization

        const D& current_descent = optional_current_descent.value();
        
        pud_node_id current_node = current_descent.node;
        
        if (check_node_leaf_.check_leaf(current_node))
            return current_node;

        const auto& children = get_node_children_.get(current_node);
        
        descend(current_descent, children.begin(), children.end());
    }

    return std::nullopt;
}

template<typename D, typename NI, typename ICNL, typename IGCN, typename IDESC, typename IGCSI>
std::optional<D> pud_witness_search_head<D, NI, ICNL, IGCN, IDESC, IGCSI>::try_replace_sibling(const D& parent_descent, NI& next_sibling_it, NI end_sibling_it) {
    while (next_sibling_it != end_sibling_it) {
        if (auto next_sibling_descent =
            descend_.descend(
                parent_descent, *(next_sibling_it++))) {
            return next_sibling_descent;
        }
    }
    return std::nullopt;
}

template<typename D, typename NI, typename ICNL, typename IGCN, typename IDESC, typename IGCSI>
void pud_witness_search_head<D, NI, ICNL, IGCN, IDESC, IGCSI>::descend(D parent_descent, NI next_sibling_it, NI end_sibling_it) {
    frame_stack_.push_back({
        .descent = std::nullopt,
        .parent_descent = parent_descent,
        .next_sibling_it = next_sibling_it,
        .end_sibling_it = end_sibling_it
    });
}

template<typename D, typename NI, typename ICNL, typename IGCN, typename IDESC, typename IGCSI>
void pud_witness_search_head<D, NI, ICNL, IGCN, IDESC, IGCSI>::ascend() {
    frame_stack_.pop_back();
    if (!frame_stack_.empty())
        frame_stack_.back().descent = std::nullopt;
}

#endif
