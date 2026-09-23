#ifndef PUD_NODE_WALKER_HPP
#define PUD_NODE_WALKER_HPP

#include "value_objects/pud_rule_id.hpp"
#include "value_objects/om_interval.hpp"

template<typename Node>
struct pud_node_walker {
    pud_node_walker(const Node& root, om_interval root_interval);
    
};

#endif
