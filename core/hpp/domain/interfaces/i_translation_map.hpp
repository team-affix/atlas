#ifndef I_TRANSLATION_MAP_HPP
#define I_TRANSLATION_MAP_HPP

#include <cstdint>

struct i_translation_map {
    virtual ~i_translation_map() = default;
    virtual void insert(uint32_t, uint32_t) = 0;
    virtual bool contains(uint32_t) const = 0;
    virtual uint32_t& at(uint32_t) = 0;
    virtual const uint32_t& at(uint32_t) const = 0;
    virtual void erase(uint32_t) = 0;
    virtual void clear() = 0;
};

#endif
