#ifndef PUD_ROOTS_HPP
#define PUD_ROOTS_HPP

#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/pud_node_id.hpp"

struct pud_roots {
    coroutine<pud_node_id, void> iterate_roots();
    void register_root(pud_node_id root);
private:
    std::vector<pud_node_id> roots_;
};

#endif
