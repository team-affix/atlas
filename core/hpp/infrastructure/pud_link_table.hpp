#ifndef PUD_LINK_TABLE_HPP
#define PUD_LINK_TABLE_HPP

#include <cstdint>
#include <unordered_map>
#include "value_objects/pud_module_id.hpp"

struct pud_link_table {
    pud_link_table();
    void insert_link(pud_module_id module, uint32_t local_functor_id, uint32_t global_functor_id);
    bool contains_module(pud_module_id module) const;
    bool contains_global_functor_id(pud_module_id module, uint32_t global_functor_id) const;
    uint32_t get_global_functor_id_from_local_functor_id(pud_module_id module, uint32_t local_functor_id) const;
    uint32_t get_local_functor_id_from_global_functor_id(pud_module_id module, uint32_t global_functor_id) const;
private:
    struct module_links {
        std::unordered_map<uint32_t, uint32_t> global_by_local;
        std::unordered_map<uint32_t, uint32_t> local_by_global;
    };

    std::unordered_map<pud_module_id, module_links> rows_;
};

#endif
