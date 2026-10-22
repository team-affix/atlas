#ifndef PUD_NODE_ID_SEQUENCER_HPP
#define PUD_NODE_ID_SEQUENCER_HPP

#include "value_objects/pud_node_id.hpp"

struct pud_node_id_sequencer {
    pud_node_id_sequencer();
    pud_node_id next();
    pud_node_id peek() const;
private:
    pud_node_id next_;
};

#endif
