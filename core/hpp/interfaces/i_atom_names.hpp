#ifndef I_ATOM_NAMES_HPP
#define I_ATOM_NAMES_HPP

#include <cstdint>
#include <string>

struct i_atom_names {
    virtual ~i_atom_names() = default;
    virtual bool is_named(uint32_t) const = 0;
    virtual const std::string& name(uint32_t) const = 0;
    virtual void set_name(uint32_t, const std::string&) = 0;
};

#endif
