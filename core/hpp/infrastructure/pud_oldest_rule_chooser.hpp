#ifndef PUD_OLDEST_RULE_CHOOSER_HPP
#define PUD_OLDEST_RULE_CHOOSER_HPP

#include <vector>
#include "value_objects/pud_node_id.hpp"

// Chooses the oldest leaf, which grows each tree breadth-first.
struct pud_oldest_rule_chooser {
    pud_node_id choose_rule(const std::vector<pud_node_id>& leaves) const;
};

#endif
