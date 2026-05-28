#ifndef VAR_NAMES_HPP
#define VAR_NAMES_HPP

#include <unordered_map>
#include "interfaces/i_var_names.hpp"

struct var_names : i_var_names {
    var_names();
    bool is_named(uint32_t) const override;
    const std::string& name(uint32_t) const override;
    void set_name(uint32_t, const std::string&) override;
private:
    std::unordered_map<uint32_t, std::string> names;
};

#endif
