#ifndef BT_CDCL_PAIR_KEY_HASH_HPP
#define BT_CDCL_PAIR_KEY_HASH_HPP

#include <cstddef>
#include "value_objects/bt_cdcl_pair_key.hpp"

struct bt_cdcl_pair_key_hash {
    size_t operator()(const bt_cdcl_pair_key& key) const noexcept;
};

#endif
