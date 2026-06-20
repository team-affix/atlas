#ifndef FUNCTOR_NAMES_HPP
#define FUNCTOR_NAMES_HPP

#include <cstdint>
#include <string>
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

inline functor_names::functor_names() {
    names.insert({k_nil_functor_id, "nil"});
    names.insert({k_cons_functor_id, "cons"});
}

inline bool functor_names::is_named(uint32_t id) const {
    return names.contains(id);
}

inline const std::string& functor_names::name(uint32_t id) const {
    return names.at(id);
}

inline void functor_names::set_name(uint32_t id, const std::string& name) {
    names.insert({id, name});
}

#endif
