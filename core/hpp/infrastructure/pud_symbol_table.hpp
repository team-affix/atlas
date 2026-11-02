#ifndef PUD_SYMBOL_TABLE_HPP
#define PUD_SYMBOL_TABLE_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct pud_symbol_table {
    pud_symbol_table();
    uint32_t intern_functor_name(const std::string& name);
    const std::string& get_name_by_global_functor_id(uint32_t global_functor_id) const;
private:
    std::unordered_map<std::string, uint32_t> global_by_name_;
    std::vector<std::string> name_by_global_;
};

#endif
