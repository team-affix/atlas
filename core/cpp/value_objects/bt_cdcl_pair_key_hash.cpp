#include "value_objects/bt_cdcl_pair_key_hash.hpp"

#include <functional>

size_t bt_cdcl_pair_key_hash::operator()(const bt_cdcl_pair_key& key) const noexcept {
    size_t seed = std::hash<bt_cdcl_factor*>{}(key.left);
    const size_t value = std::hash<bt_cdcl_factor*>{}(key.right);
    return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
