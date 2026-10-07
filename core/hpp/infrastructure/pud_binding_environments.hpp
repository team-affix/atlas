#ifndef PUD_BINDING_ENVIRONMENTS_HPP
#define PUD_BINDING_ENVIRONMENTS_HPP

#include <unordered_map>
#include <immer/map.hpp>
#include <cstdint>
#include "value_objects/pud_node.hpp"
#include "value_objects/framed_expr.hpp"

struct pud_binding_environments {
    using map_t = immer::map<uint32_t, framed_expr>;
    void record(const pud_node* node, map_t bindings);
    const map_t& query(const pud_node* node) const;
private:
    std::unordered_map<const pud_node*, map_t> envs_;
};

#endif
