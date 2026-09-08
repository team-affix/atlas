#ifndef ORDER_MAINTENANCE_HPP
#define ORDER_MAINTENANCE_HPP

#include <cstdint>
#include <unordered_map>
#include "value_objects/om_interval.hpp"

struct order_maintenance {
    order_maintenance();
    ~order_maintenance();
    om_interval allocate_root();
    om_interval allocate_child_of(const om_interval& parent);
private:
    struct node {
        uint64_t rank;
        node* prev;
        node* next;
    };

    om_label insert_before(node* next_node);
    void relabel_segment(node* start, node* end, uint64_t count);

    node* head_;
    node* tail_;
    std::unordered_map<const uint64_t*, node*> node_by_rank_ptr_;
};

#endif
