#ifndef PUD_SYMBOL_TABLE_HPP
#define PUD_SYMBOL_TABLE_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

struct pud_symbol_table {
    pud_symbol_table();
    uint32_t intern_functor_name(const std::string& name);
    uint32_t get_global_functor_id_by_name(const std::string& name) const;
    bool contains_named_functor_id(uint32_t global_functor_id) const;
    const std::string& get_name_by_global_functor_id(uint32_t global_functor_id) const;
private:
    std::unordered_map<std::string, uint32_t> global_by_name_;
    std::unordered_map<uint32_t, std::string> name_by_global_;
};

#endif
