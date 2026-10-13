#ifndef UBS_BIND_MAP_HARNESS_HPP
#define UBS_BIND_MAP_HARNESS_HPP

#include <cstdint>
#include <deque>
#include <unordered_map>
#include "infrastructure/globalizer.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/expr.hpp"

struct ubs_var_factory {
    std::deque<expr> exprs;

    const expr* make(uint32_t global_index) {
        exprs.push_back(expr{expr::var{global_index}});
        return &exprs.back();
    }
};

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

using ubs_bind_map_t = ubs_bind_map<
    ubs_var_factory,
    ubs_std_parent_map,
    ubs_std_parent_map,
    ubs_std_size_map,
    ubs_std_size_map,
    ubs_std_value_map,
    ubs_std_value_map>;

struct ubs_bind_map_backing {
    ubs_var_factory vars;
    ubs_std_parent_map parents;
    ubs_std_size_map sizes;
    ubs_std_value_map values;

    ubs_bind_map_t make_map() {
        return ubs_bind_map_t{vars, parents, parents, sizes, sizes, values, values};
    }
};

struct ubs_map_snapshot {
    std::unordered_map<uint32_t, uint32_t> parents;
    std::unordered_map<uint32_t, uint32_t> sizes;
    std::unordered_map<uint32_t, framed_expr> values;
};

inline ubs_map_snapshot snapshot_ubs_maps(const ubs_bind_map_backing& backing) {
    return ubs_map_snapshot{backing.parents.map, backing.sizes.map, backing.values.map};
}

inline bool ubs_snapshots_equal(const ubs_map_snapshot& left, const ubs_map_snapshot& right) {
    return left.parents == right.parents && left.sizes == right.sizes && left.values == right.values;
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

inline uint32_t ubs_global_key(globalizer& globalize, uint32_t frame_offset, uint32_t local_index) {
    return globalize.globalize(frame_offset, local_index);
}

inline framed_expr ubs_resolve_local(
    ubs_bind_map_t& store, globalizer& globalize, uint32_t frame_offset, uint32_t local_index) {
    return store.resolve(ubs_global_key(globalize, frame_offset, local_index));
}

inline uint32_t ubs_rep_index(const framed_expr& resolved) {
    return std::get<expr::var>(resolved.skeleton->content).index;
}

#endif
