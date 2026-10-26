#include "infrastructure/ubs_bind_map.hpp"
#include <algorithm>
#include <utility>
#include "debug_assert.hpp"

ubs_bind_map::ubs_bind_map(const pud_ubs_state& state)
    : parents_(state.parents.transient())
    , classes_(state.classes.transient()) {}

ubs_class ubs_bind_map::find(uint32_t var) const {
    while (const uint32_t* parent = parents_.find(var))
        var = *parent;
    if (const ubs_class* stored = classes_.find(var))
        return *stored;
    return ubs_class{.root = var, .size = 1, .rep = var, .value = std::nullopt};
}

std::optional<framed_functor> ubs_bind_map::equate(const ubs_class& class_a, const ubs_class& class_b) {
    DEBUG_ASSERT(class_a.root != class_b.root);
    const bool a_survives = class_a.size >= class_b.size;
    const ubs_class& survivor = a_survives ? class_a : class_b;
    const ubs_class& absorbed = a_survives ? class_b : class_a;
    const bool both_bound = survivor.value.has_value() && absorbed.value.has_value();
    const bool absorbed_stored = absorbed.size > 1 || absorbed.value.has_value();
    parents_.set(absorbed.root, survivor.root);
    if (absorbed_stored)
        classes_.erase(absorbed.root);
    classes_.set(survivor.root, ubs_class{
        .root = survivor.root,
        .size = survivor.size + absorbed.size,
        .rep = std::min(survivor.rep, absorbed.rep),
        .value = survivor.value.has_value() ? survivor.value : absorbed.value,
    });
    if (!both_bound)
        return std::nullopt;
    return absorbed.value;
}

void ubs_bind_map::instantiate(const ubs_class& unbound, framed_functor value) {
    DEBUG_ASSERT(!unbound.value.has_value());
    classes_.set(unbound.root, ubs_class{
        .root = unbound.root,
        .size = unbound.size,
        .rep = unbound.rep,
        .value = value,
    });
}

pud_ubs_state ubs_bind_map::persist() && {
    return pud_ubs_state{
        .parents = std::move(parents_).persistent(),
        .classes = std::move(classes_).persistent(),
    };
}
