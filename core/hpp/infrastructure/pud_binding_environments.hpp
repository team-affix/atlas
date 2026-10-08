#ifndef PUD_BINDING_ENVIRONMENTS_HPP
#define PUD_BINDING_ENVIRONMENTS_HPP

#include <cstdint>
#include <unordered_map>
#include <immer/map.hpp>
#include "value_objects/pud_node_id.hpp"
#include "value_objects/framed_expr.hpp"

struct pud_binding_environments {
    using map_t = immer::map<uint32_t, framed_expr>;
    void record(pud_node_id id, map_t bindings);
    const map_t& query(pud_node_id id) const;
private:
    std::unordered_map<pud_node_id, map_t> envs_;
};

#endif
