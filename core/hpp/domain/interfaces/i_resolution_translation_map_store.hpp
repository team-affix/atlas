#ifndef I_RESOLUTION_TRANSLATION_MAP_STORE_HPP
#define I_RESOLUTION_TRANSLATION_MAP_STORE_HPP

#include <unordered_map>
#include <cstdint>
#include "i_resolution_store.hpp"

using translation_map = std::unordered_map<uint32_t, uint32_t>;

struct i_resolution_translation_map_store : i_resolution_store<translation_map> {
    virtual ~i_resolution_translation_map_store() = default;
};

#endif
