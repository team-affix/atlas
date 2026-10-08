#include "infrastructure/pud_roots.hpp"
#include "debug_assert.hpp"

coroutine<const pud_node*, void> pud_roots::iterate_roots() {
    for (const pud_node* root : roots_)
        co_yield root;
}

void pud_roots::register_root(const pud_node* root) {
    DEBUG_ASSERT(root != nullptr);
    roots_.push_back(root);
}
