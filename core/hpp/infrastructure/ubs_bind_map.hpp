#ifndef UBS_BIND_MAP_HPP
#define UBS_BIND_MAP_HPP

#include <cstdint>
#include <utility>
#include <variant>
#include "value_objects/expr.hpp"
#include "debug_assert.hpp"

// Union-find over global variable indices: union by size, no path compression.
// parent: var -> var, links only ever connect variables to variables.
// size:   root -> class size. No entry = 1.
// value:  root -> hash-consed expr in global indices: a functor (class bound)
//         or the rep var (class unbound; rep = minimum index in the class).
//         No entry = unbound, rep is the root.
// Entries at non-roots are stale and never read, so nothing is ever erased.
// The bind map stores values as given; normalizing them is the caller's job.
template<
    typename IMakeVar,
    typename IGetParent,
    typename ISetParent,
    typename IGetSize,
    typename ISetSize,
    typename IGetValue,
    typename ISetValue>
struct ubs_bind_map {
    ubs_bind_map(
        IMakeVar& make_var,
        IGetParent& get_parent,
        ISetParent& set_parent,
        IGetSize& get_size,
        ISetSize& set_size,
        IGetValue& get_value,
        ISetValue& set_value);

    // Any variable. Bound class: its functor. Unbound class: its rep var.
    const expr* resolve(uint32_t var);
    // Any two variables in different unbound classes. Precondition: roots differ.
    void equate(uint32_t var_a, uint32_t var_b);
    // Any variable. Preconditions: its class is unbound; functor is a functor.
    void instantiate(uint32_t var, const expr* functor);

private:
    uint32_t find(uint32_t var);
    uint32_t rep(uint32_t root); // DEBUG_ASSERT: class is unbound

    IMakeVar& make_var_;
    IGetParent& get_parent_;
    ISetParent& set_parent_;
    IGetSize& get_size_;
    ISetSize& set_size_;
    IGetValue& get_value_;
    ISetValue& set_value_;
};

#define UBS_BIND_MAP_TEMPLATE                                                        \
    template<typename IMakeVar, typename IGetParent, typename ISetParent,           \
             typename IGetSize, typename ISetSize, typename IGetValue, typename ISetValue>
#define UBS_BIND_MAP ubs_bind_map<IMakeVar, IGetParent, ISetParent, IGetSize, ISetSize, IGetValue, ISetValue>

UBS_BIND_MAP_TEMPLATE
UBS_BIND_MAP::ubs_bind_map(
    IMakeVar& make_var,
    IGetParent& get_parent,
    ISetParent& set_parent,
    IGetSize& get_size,
    ISetSize& set_size,
    IGetValue& get_value,
    ISetValue& set_value)
    : make_var_(make_var)
    , get_parent_(get_parent)
    , set_parent_(set_parent)
    , get_size_(get_size)
    , set_size_(set_size)
    , get_value_(get_value)
    , set_value_(set_value) {}

UBS_BIND_MAP_TEMPLATE
uint32_t UBS_BIND_MAP::find(uint32_t var) {
    while (const uint32_t* parent = get_parent_.get_parent(var))
        var = *parent;
    return var;
}

UBS_BIND_MAP_TEMPLATE
uint32_t UBS_BIND_MAP::rep(uint32_t root) {
    const expr* const* value = get_value_.get_value(root);
    if (!value)
        return root;
    const expr::var* rep_var = std::get_if<expr::var>(&(*value)->content);
    DEBUG_ASSERT(rep_var != nullptr);
    return rep_var->index;
}

UBS_BIND_MAP_TEMPLATE
const expr* UBS_BIND_MAP::resolve(uint32_t var) {
    const uint32_t root = find(var);
    if (const expr* const* value = get_value_.get_value(root))
        return *value;
    return make_var_.make(root);
}

UBS_BIND_MAP_TEMPLATE
void UBS_BIND_MAP::equate(uint32_t var_a, uint32_t var_b) {
    uint32_t root_a = find(var_a);
    uint32_t root_b = find(var_b);
    
    DEBUG_ASSERT(root_a != root_b);

    const uint32_t rep_a = rep(root_a);
    const uint32_t rep_b = rep(root_b);
    const uint32_t* size_a_entry = get_size_.get_size(root_a);
    const uint32_t* size_b_entry = get_size_.get_size(root_b);
    const uint32_t size_a = size_a_entry ? *size_a_entry : 1u;
    const uint32_t size_b = size_b_entry ? *size_b_entry : 1u;

    if (size_a < size_b)
        std::swap(root_a, root_b); // root_a is now the larger root (survivor)

    set_parent_.set_parent(root_b, root_a);
    set_size_.set_size(root_a, size_a + size_b);
    set_value_.set_value(root_a, make_var_.make(rep_a < rep_b ? rep_a : rep_b));
}

UBS_BIND_MAP_TEMPLATE
void UBS_BIND_MAP::instantiate(uint32_t var, const expr* functor) {
    DEBUG_ASSERT(std::holds_alternative<expr::functor>(functor->content));
    const uint32_t root = find(var);
    rep(root);
    set_value_.set_value(root, functor);
}

#undef UBS_BIND_MAP
#undef UBS_BIND_MAP_TEMPLATE

#endif
