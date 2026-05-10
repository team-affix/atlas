#ifndef I_TRANSLATION_MAP_HPP
#define I_TRANSLATION_MAP_HPP

#include <cstdint>
#include "i_map.hpp"

struct i_translation_map : i_map<uint32_t, uint32_t> {
    virtual ~i_translation_map() = default;
};

#endif
