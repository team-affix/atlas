#ifndef PUD_NORMALIZER_HPP
#define PUD_NORMALIZER_HPP

#include <cstdint>
#include <optional>
#include <unordered_map>
#include "infrastructure/hierarchical_bind_map.hpp"
#include "infrastructure/normalizer.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "debug_assert.hpp"

template<typename IGlobalize,
         typename IRecordBinding,
         typename IQueryBinding,
         typename IMakeVar,
         typename IMakeFunctor>
struct pud_normalizer {
    pud_normalizer(IGlobalize& globalize,
                   IRecordBinding& record_binding,
                   IQueryBinding& query_binding,
                   IMakeVar& make_var,
                   IMakeFunctor& make_functor);
    void set_normalization_environment(om_interval interval, uint32_t cutoff);
    const expr* normalize(framed_expr fe,
                          std::unordered_map<uint32_t, uint32_t>& translation);
private:
    using bind_map_t = hierarchical_bind_map<IGlobalize, IRecordBinding, IQueryBinding>;
    using normalizer_t = normalizer<IGlobalize, IMakeFunctor, IMakeVar, bind_map_t>;

    IGlobalize& globalize_;
    IRecordBinding& record_binding_;
    IQueryBinding& query_binding_;
    IMakeVar& make_var_;
    IMakeFunctor& make_functor_;
    std::optional<bind_map_t> bind_map_;
    std::optional<normalizer_t> normalizer_;
    uint32_t cutoff_;
};

template<typename IG, typename IRB, typename IQB, typename IMV, typename IMF>
pud_normalizer<IG, IRB, IQB, IMV, IMF>::pud_normalizer(
        IG& globalize,
        IRB& record_binding,
        IQB& query_binding,
        IMV& make_var,
        IMF& make_functor)
    : globalize_(globalize)
    , record_binding_(record_binding)
    , query_binding_(query_binding)
    , make_var_(make_var)
    , make_functor_(make_functor)
    , bind_map_()
    , normalizer_()
    , cutoff_(0) {}

template<typename IG, typename IRB, typename IQB, typename IMV, typename IMF>
void pud_normalizer<IG, IRB, IQB, IMV, IMF>::set_normalization_environment(
        om_interval interval, uint32_t cutoff) {
    normalizer_.reset();
    bind_map_.reset();
    bind_map_.emplace(globalize_, record_binding_, query_binding_, interval);
    normalizer_.emplace(globalize_, make_functor_, make_var_, *bind_map_);
    cutoff_ = cutoff;
}

template<typename IG, typename IRB, typename IQB, typename IMV, typename IMF>
const expr* pud_normalizer<IG, IRB, IQB, IMV, IMF>::normalize(
        framed_expr fe,
        std::unordered_map<uint32_t, uint32_t>& translation) {
    DEBUG_ASSERT(normalizer_.has_value());
    return normalizer_->normalize(fe, cutoff_, translation);
}

#endif
