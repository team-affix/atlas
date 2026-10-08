#ifndef PUD_ROOTS_HPP
#define PUD_ROOTS_HPP

#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/pud_node.hpp"

struct pud_roots {
    coroutine<const pud_node*, void> iterate_roots();
    void register_root(const pud_node* root);
private:
    std::vector<const pud_node*> roots_;
};

#endif
