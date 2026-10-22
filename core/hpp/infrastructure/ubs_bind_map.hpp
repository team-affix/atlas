#ifndef UBS_BIND_MAP_HPP
#define UBS_BIND_MAP_HPP

#include <cstdint>
#include <optional>
#include <utility>
#include "value_objects/framed_expr.hpp"
#include "debug_assert.hpp"

template<
    typename IGetParent,
    typename ISetParent,
    typename IGetSize,
    typename ISetSize,
    typename IGetRep,
    typename ISetRep,
    typename IGetValue,
    typename ISetValue>
struct ubs_bind_map {
    ubs_bind_map(
        IGetParent& get_parent,
        ISetParent& set_parent,
        IGetSize& get_size,
        ISetSize& set_size,
        IGetRep& get_rep,
        ISetRep& set_rep,
        IGetValue& get_value,
        ISetValue& set_value);

    uint32_t find(uint32_t var);
    uint32_t find_rep(uint32_t var);
    std::optional<framed_expr> find_value(uint32_t var);
    void equate(uint32_t var_a, uint32_t var_b);
    void instantiate(uint32_t var, framed_expr value);

private:
    uint32_t size_of(uint32_t root);
    uint32_t rep_of(uint32_t root);

    IGetParent& get_parent_;
    ISetParent& set_parent_;
    IGetSize& get_size_;
    ISetSize& set_size_;
    IGetRep& get_rep_;
    ISetRep& set_rep_;
    IGetValue& get_value_;
    ISetValue& set_value_;
};

#define UBS_BIND_MAP_TEMPLATE                                                                     \
    template<typename IGetParent, typename ISetParent, typename IGetSize, typename ISetSize,     \
             typename IGetRep, typename ISetRep, typename IGetValue, typename ISetValue>
#define UBS_BIND_MAP ubs_bind_map<IGetParent, ISetParent, IGetSize, ISetSize, IGetRep, ISetRep, IGetValue, ISetValue>

UBS_BIND_MAP_TEMPLATE
UBS_BIND_MAP::ubs_bind_map(
    IGetParent& get_parent,
    ISetParent& set_parent,
    IGetSize& get_size,
    ISetSize& set_size,
    IGetRep& get_rep,
    ISetRep& set_rep,
    IGetValue& get_value,
    ISetValue& set_value)
    : get_parent_(get_parent)
    , set_parent_(set_parent)
    , get_size_(get_size)
    , set_size_(set_size)
    , get_rep_(get_rep)
    , set_rep_(set_rep)
    , get_value_(get_value)
    , set_value_(set_value) {}

UBS_BIND_MAP_TEMPLATE
uint32_t UBS_BIND_MAP::find(uint32_t var) {
    while (const uint32_t* parent = get_parent_.get_parent(var))
        var = *parent;
    return var;
}

UBS_BIND_MAP_TEMPLATE
uint32_t UBS_BIND_MAP::find_rep(uint32_t var) {
    return rep_of(find(var));
}

UBS_BIND_MAP_TEMPLATE
std::optional<framed_expr> UBS_BIND_MAP::find_value(uint32_t var) {
    const framed_expr* value = get_value_.get_value(find(var));
    if (!value)
        return std::nullopt;
    return *value;
}

UBS_BIND_MAP_TEMPLATE
void UBS_BIND_MAP::equate(uint32_t var_a, uint32_t var_b) {
    uint32_t survivor = find(var_a);
    uint32_t absorbed = find(var_b);
    DEBUG_ASSERT(survivor != absorbed);
    if (size_of(survivor) < size_of(absorbed))
        std::swap(survivor, absorbed);

    const uint32_t class_size = size_of(survivor) + size_of(absorbed);
    const uint32_t survivor_rep = rep_of(survivor);
    const uint32_t absorbed_rep = rep_of(absorbed);
    const bool survivor_bound = get_value_.get_value(survivor) != nullptr;
    const framed_expr* absorbed_value = get_value_.get_value(absorbed);
    const std::optional<framed_expr> moved_value =
        survivor_bound || !absorbed_value ? std::nullopt : std::optional<framed_expr>(*absorbed_value);

    set_parent_.set_parent(absorbed, survivor);
    set_size_.set_size(survivor, class_size);
    if (absorbed_rep < survivor_rep)
        set_rep_.set_rep(survivor, absorbed_rep);
    if (moved_value.has_value())
        set_value_.set_value(survivor, moved_value.value());
}

UBS_BIND_MAP_TEMPLATE
void UBS_BIND_MAP::instantiate(uint32_t var, framed_expr value) {
    const uint32_t root = find(var);
    DEBUG_ASSERT(get_value_.get_value(root) == nullptr);
    set_value_.set_value(root, value);
}

UBS_BIND_MAP_TEMPLATE
uint32_t UBS_BIND_MAP::size_of(uint32_t root) {
    const uint32_t* size = get_size_.get_size(root);
    return size ? *size : 1u;
}

UBS_BIND_MAP_TEMPLATE
uint32_t UBS_BIND_MAP::rep_of(uint32_t root) {
    const uint32_t* rep = get_rep_.get_rep(root);
    return rep ? *rep : root;
}

#undef UBS_BIND_MAP
#undef UBS_BIND_MAP_TEMPLATE

#endif
