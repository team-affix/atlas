#ifndef PUD_LEAF_MATERIALIZER_HPP
#define PUD_LEAF_MATERIALIZER_HPP

#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_node_id.hpp"

// Reads a term written in a live leaf's variables as that leaf's bindings make it.
template<typename IGetLeafState, typename IMakeBindMap, typename IMakeNormalizer>
struct pud_leaf_materializer {
    pud_leaf_materializer(IGetLeafState& get_leaf_state, IMakeBindMap& make_bind_map, IMakeNormalizer& make_normalizer);
    const expr* materialize(pud_node_id leaf, const expr* skeleton);
private:
    IGetLeafState& get_leaf_state_;
    IMakeBindMap& make_bind_map_;
    IMakeNormalizer& make_normalizer_;
};

template<typename IGLS, typename IMBM, typename IMN>
pud_leaf_materializer<IGLS, IMBM, IMN>::pud_leaf_materializer(IGLS& get_leaf_state, IMBM& make_bind_map, IMN& make_normalizer)
    : get_leaf_state_(get_leaf_state)
    , make_bind_map_(make_bind_map)
    , make_normalizer_(make_normalizer) {}

template<typename IGLS, typename IMBM, typename IMN>
const expr* pud_leaf_materializer<IGLS, IMBM, IMN>::materialize(pud_node_id leaf, const expr* skeleton) {
    auto&& bind_map = make_bind_map_.make_bind_map(get_leaf_state_.get(leaf).bindings);
    auto&& normalizer = make_normalizer_.make_normalizer(bind_map);
    return normalizer.normalize(framed_expr{skeleton, 0});
}

#endif
