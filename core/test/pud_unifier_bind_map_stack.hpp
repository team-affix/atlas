#ifndef PUD_UNIFIER_BIND_MAP_STACK_HPP
#define PUD_UNIFIER_BIND_MAP_STACK_HPP

#include <unordered_map>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_normalizer.hpp"
#include "infrastructure/pud_unifier.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "ubs_bind_map_test_maps.hpp"
#include "value_objects/framed_expr.hpp"

struct ubs_pool_var_factory {
    expr_pool& pool_;

    const expr* make(uint32_t global_index) {
        return pool_.make_var(global_index);
    }
};

struct pud_pool_make_functor {
    expr_pool& pool_;

    const expr* make(uint32_t id, const std::vector<const expr*>& args) {
        return pool_.make_functor(id, args);
    }
};

struct ubs_bind_map_expr_pool_backing {
    expr_pool pool;
    ubs_pool_var_factory vars{pool};
    std::unordered_map<uint32_t, uint32_t> parents;
    std::unordered_map<uint32_t, uint32_t> sizes;
    std::unordered_map<uint32_t, const expr*> values;

    struct parent_map {
        std::unordered_map<uint32_t, uint32_t>& map;
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

    struct size_map {
        std::unordered_map<uint32_t, uint32_t>& map;
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

    struct value_map {
        std::unordered_map<uint32_t, const expr*>& map;
        const expr* const* get_value(uint32_t root) const {
            const auto entry = map.find(root);
            if (entry == map.end())
                return nullptr;
            return &entry->second;
        }
        void set_value(uint32_t root, const expr* value) {
            map[root] = value;
        }
    };

    parent_map parent_access{parents};
    size_map size_access{sizes};
    value_map value_access{values};

    using bind_map_t = ubs_bind_map<
        ubs_pool_var_factory,
        parent_map,
        parent_map,
        size_map,
        size_map,
        value_map,
        value_map>;

    bind_map_t make_bind_map() {
        return bind_map_t{vars, parent_access, parent_access, size_access, size_access, value_access, value_access};
    }
};

struct pud_unifier_bind_map_stack {
    globalizer globalize;
    ubs_bind_map_expr_pool_backing bind_backing;
    ubs_bind_map_expr_pool_backing::bind_map_t bind_map{bind_backing.make_bind_map()};
    pud_pool_make_functor make_functor{bind_backing.pool};
    global_normalizer<globalizer, pud_pool_make_functor, ubs_bind_map_expr_pool_backing::bind_map_t> normalizer{
        globalize, make_functor, bind_map};

    using pud_unifier_t = unifier<
        globalizer,
        global_normalizer<globalizer, pud_pool_make_functor, ubs_bind_map_expr_pool_backing::bind_map_t>,
        ubs_bind_map_expr_pool_backing::bind_map_t,
        ubs_bind_map_expr_pool_backing::bind_map_t,
        ubs_bind_map_expr_pool_backing::bind_map_t>;

    pud_unifier_t pud_unifier{globalize, normalizer, bind_map, bind_map, bind_map};
};

inline framed_expr pud_fe(const expr* skeleton, uint32_t frame_offset) {
    return framed_expr{skeleton, frame_offset};
}

inline ubs_map_snapshot snapshot_bind_backing(const ubs_bind_map_expr_pool_backing& backing) {
    return ubs_map_snapshot{backing.parents, backing.sizes, backing.values};
}

inline bool pud_expr_bindings_equal(const expr* left, const expr* right) {
    if (left == right)
        return true;
    if (left->content.index() != right->content.index())
        return false;
    if (const expr::var* left_var = std::get_if<expr::var>(&left->content)) {
        const expr::var* right_var = std::get_if<expr::var>(&right->content);
        return left_var->index == right_var->index;
    }
    const expr::functor& left_functor = std::get<expr::functor>(left->content);
    const expr::functor& right_functor = std::get<expr::functor>(right->content);
    if (left_functor.id != right_functor.id || left_functor.args.size() != right_functor.args.size())
        return false;
    for (size_t arg = 0; arg < left_functor.args.size(); ++arg) {
        if (!pud_expr_bindings_equal(left_functor.args[arg], right_functor.args[arg]))
            return false;
    }
    return true;
}

#endif
