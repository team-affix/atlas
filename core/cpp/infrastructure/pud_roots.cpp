#include "infrastructure/pud_roots.hpp"

coroutine<pud_node_id, void> pud_roots::iterate_roots() {
    for (pud_node_id root : roots_)
        co_yield root;
}

void pud_roots::register_root(pud_node_id root) {
    roots_.push_back(root);
}
