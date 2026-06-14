#ifndef ATOM_NAMES_HPP
#define ATOM_NAMES_HPP

#include <cstdint>
#include <unordered_map>
#include "interfaces/i_atom_names.hpp"

inline constexpr uint32_t k_nil_atom_id = 0;
inline constexpr uint32_t k_cons_atom_id = 1;
inline constexpr uint32_t k_first_user_atom_id = 2;

struct atom_names : i_atom_names {
    atom_names();
    bool is_named(uint32_t) const override;
    const std::string& name(uint32_t) const override;
    void set_name(uint32_t, const std::string&) override;
private:
    std::unordered_map<uint32_t, std::string> names;
};

#endif
