#ifndef VAR_NAMES_HPP
#define VAR_NAMES_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

struct var_names {
    var_names();
    bool is_named(uint32_t) const;
    const std::string& name(uint32_t) const;
    void set_name(uint32_t, const std::string&);
private:
    std::unordered_map<uint32_t, std::string> names_;
};

#endif
