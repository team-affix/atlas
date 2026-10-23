#ifndef PUD_NODE_STATE_HPP
#define PUD_NODE_STATE_HPP

// A node starts as a leaf; expanding it makes it interior, and refuting a leaf rules it out for good.
enum class pud_node_state {
    leaf,
    expanded,
    refuted,
};

#endif
