#ifndef PUD_CALL_SITES_HPP
#define PUD_CALL_SITES_HPP

#include <cstddef>
#include <unordered_map>
#include "value_objects/pud_node.hpp"

struct pud_call_sites {
    size_t get(const pud_node* node) const;
    void store(const pud_node* node, size_t call_site);
private:
    std::unordered_map<const pud_node*, size_t> call_sites_;
};

#endif
