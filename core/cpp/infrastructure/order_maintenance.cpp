#include "infrastructure/order_maintenance.hpp"

#include <cstdint>
#include <limits>
#include <unordered_map>
#include "debug_assert.hpp"

// ---------------------------------------------------------------------------
// order_maintenance
//
// Maintains a doubly-linked list of nodes, each carrying a uint64_t rank.
// Two sentinel nodes (head_, tail_) permanently bookend the list with ranks 0
// and UINT64_MAX respectively.  Every real node's rank sits strictly between
// its predecessor's and successor's ranks.
//
// When inserting a new node and the gap between neighbors is too small, we
// call relabel_segment to expand a window around the crowded area and evenly
// redistribute ranks within it, restoring breathing room.  The window expands
// geometrically until the per-gap step is at least 2.
//
// We keep node_by_rank_ptr_ to let allocate_child_of recover the node*
// from an om_label's rank_ptr() without any reinterpret_cast.
// ---------------------------------------------------------------------------

namespace {
    constexpr uint64_t k_rank_max = std::numeric_limits<uint64_t>::max();
}

order_maintenance::order_maintenance()
    : head_(new node{0, nullptr, nullptr})
    , tail_(new node{k_rank_max, nullptr, nullptr}) {
    head_->next = tail_;
    tail_->prev = head_;
}

order_maintenance::~order_maintenance() {
    node* current = head_;
    while (current != nullptr) {
        node* next_node = current->next;
        delete current;
        current = next_node;
    }
}

void order_maintenance::relabel_segment(node* start, node* end, uint64_t count) {
    // Expand window outward (but never past sentinels) until the available
    // rank span supports at least (count + 1) equal-sized steps of size >= 2.
    while (true) {
        const uint64_t span = end->rank - start->rank;
        if (count == 0 || span / (count + 1) >= 2)
            break;
        const bool can_expand_left  = (start->prev != nullptr);
        const bool can_expand_right = (end->next   != nullptr);
        // If both sentinels are already in the window, the span is UINT64_MAX.
        // span / (count + 1) < 2 would require count > 2^62, which is
        // impossible in practice; guard anyway to prevent an infinite loop.
        DEBUG_ASSERT(can_expand_left || can_expand_right);
        if (!can_expand_left && !can_expand_right)
            break;
        if (can_expand_left) {
            start = start->prev;
            ++count;
        }
        if (can_expand_right) {
            end = end->next;
            ++count;
        }
    }

    const uint64_t span = end->rank - start->rank;
    const uint64_t step = span / (count + 1);

    node* current = start->next;
    uint64_t assigned = 1;
    while (current != end) {
        current->rank = start->rank + step * assigned;
        ++assigned;
        current = current->next;
    }
}

om_label order_maintenance::insert_before(node* next_node) {
    node* prev_node = next_node->prev;

    if (next_node->rank - prev_node->rank < 2) {
        // No integer strictly between the two neighbors — relabel first.
        // The gap to fill is exactly 1 slot between prev_node and next_node.
        relabel_segment(prev_node, next_node, 1);
    }

    const uint64_t new_rank =
        prev_node->rank + (next_node->rank - prev_node->rank) / 2;

    node* new_node = new node{new_rank, prev_node, next_node};
    prev_node->next = new_node;
    next_node->prev = new_node;

    node_by_rank_ptr_[&new_node->rank] = new_node;
    return om_label(&new_node->rank);
}

om_interval order_maintenance::allocate_root() {
    // Both open and close are inserted before tail_, so they appear at the
    // end of the current list in the order: ... [open] [close] [tail].
    om_label open_label  = insert_before(tail_);
    om_label close_label = insert_before(tail_);
    return om_interval{open_label, close_label};
}

om_interval order_maintenance::allocate_child_of(const om_interval& parent) {
    // Insert new_open and new_close immediately before parent's close node,
    // yielding: ... [parent.open] ... [new_open] [new_close] [parent.close] ...
    node* parent_close_node = node_by_rank_ptr_.at(parent.close.rank_ptr());
    om_label new_open  = insert_before(parent_close_node);
    om_label new_close = insert_before(parent_close_node);
    return om_interval{new_open, new_close};
}
