#ifndef UBS_BIND_MAP_TEST_MAPS_HPP
#define UBS_BIND_MAP_TEST_MAPS_HPP

#include <cstdint>
#include <unordered_map>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

struct ubs_std_parent_map {
    std::unordered_map<uint32_t, uint32_t> map;

    const uint32_t* get_parent(uint32_t var) const {
        const auto entry = map.find(var);
        if (entry == map.end())
            return nullptr;
        return &entry->second;
    }

    void set_parent(uint32_t child, uint32_t parent) {
        map[child] = parent;
    }
};

struct ubs_std_size_map {
    std::unordered_map<uint32_t, uint32_t> map;

    const uint32_t* get_size(uint32_t root) const {
        const auto entry = map.find(root);
        if (entry == map.end())
            return nullptr;
        return &entry->second;
    }

    void set_size(uint32_t root, uint32_t class_size) {
        map[root] = class_size;
    }
};

struct ubs_std_rep_map {
    std::unordered_map<uint32_t, uint32_t> map;

    const uint32_t* get_rep(uint32_t root) const {
        const auto entry = map.find(root);
        if (entry == map.end())
            return nullptr;
        return &entry->second;
    }

    void set_rep(uint32_t root, uint32_t rep) {
        map[root] = rep;
    }
};

struct ubs_std_value_map {
    std::unordered_map<uint32_t, framed_expr> map;

    const framed_expr* get_value(uint32_t root) const {
        const auto entry = map.find(root);
        if (entry == map.end())
            return nullptr;
        return &entry->second;
    }

    void set_value(uint32_t root, framed_expr value) {
        map[root] = value;
    }
};

struct ubs_map_snapshot {
    std::unordered_map<uint32_t, uint32_t> parents;
    std::unordered_map<uint32_t, uint32_t> sizes;
    std::unordered_map<uint32_t, uint32_t> reps;
    std::unordered_map<uint32_t, framed_expr> values;
};

inline ubs_map_snapshot snapshot_ubs_maps(
    const ubs_std_parent_map& parents,
    const ubs_std_size_map& sizes,
    const ubs_std_rep_map& reps,
    const ubs_std_value_map& values) {
    return ubs_map_snapshot{parents.map, sizes.map, reps.map, values.map};
}

inline bool ubs_snapshots_equal(const ubs_map_snapshot& left, const ubs_map_snapshot& right) {
    return left.parents == right.parents && left.sizes == right.sizes && left.reps == right.reps
        && left.values == right.values;
}

inline uint32_t ubs_find_root(const ubs_std_parent_map& parents, uint32_t var) {
    while (const uint32_t* parent = parents.get_parent(var))
        var = *parent;
    return var;
}

inline uint32_t ubs_parent_chain_depth(const ubs_std_parent_map& parents, uint32_t var) {
    uint32_t depth = 0;
    while (const uint32_t* parent = parents.get_parent(var)) {
        var = *parent;
        ++depth;
    }
    return depth;
}

#endif
