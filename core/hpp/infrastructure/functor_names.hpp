#ifndef FUNCTOR_NAMES_HPP
#define FUNCTOR_NAMES_HPP

#include <cstdint>
#include <unordered_map>
#include "interfaces/i_functor_names.hpp"

inline constexpr uint32_t k_nil_functor_id = 0;
inline constexpr uint32_t k_cons_functor_id = 1;
inline constexpr uint32_t k_first_user_functor_id = 2;

struct functor_names : i_functor_names {
    functor_names();
    bool is_named(uint32_t) const override;
    const std::string& name(uint32_t) const override;
    void set_name(uint32_t, const std::string&) override;
private:
    std::unordered_map<uint32_t, std::string> names;
};

#endif
