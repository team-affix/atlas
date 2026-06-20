#ifndef FUNCTOR_NAMES_HPP
#define FUNCTOR_NAMES_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

inline constexpr uint32_t k_nil_functor_id = 0;
inline constexpr uint32_t k_cons_functor_id = 1;
inline constexpr uint32_t k_first_user_functor_id = 2;

struct functor_names {
    functor_names();
    bool is_named(uint32_t) const;
    const std::string& name(uint32_t) const;
    void set_name(uint32_t, const std::string&);
private:
    std::unordered_map<uint32_t, std::string> names;
};

#endif
