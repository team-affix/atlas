#ifndef PUD_CALL_SITES_HPP
#define PUD_CALL_SITES_HPP

#include <cstddef>
#include <unordered_map>
#include "value_objects/pud_node_id.hpp"

struct pud_call_sites {
    size_t get(pud_node_id id) const;
    void store(pud_node_id id, size_t call_site);
private:
    std::unordered_map<pud_node_id, size_t> call_sites_;
};

#endif
