#ifndef HIERARCHICAL_BIND_MAP_HPP
#define HIERARCHICAL_BIND_MAP_HPP

#include <optional>
#include "value_objects/framed_expr.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_interval.hpp"
#include "debug_assert.hpp"

template<typename IGlobalize,
         typename IRecordFPArrayBinding,
         typename IQueryFPArrayBinding>
struct hierarchical_bind_map {
    hierarchical_bind_map(IGlobalize& g,
                          IRecordFPArrayBinding& record_fp,
                          IQueryFPArrayBinding& query_fp,
                          om_interval interval);
    void bind(uint32_t global_key, framed_expr value);
    framed_expr whnf(framed_expr fe);
private:
    IGlobalize&            globalizer_;
    IRecordFPArrayBinding& record_fp_;
    IQueryFPArrayBinding&  query_fp_;
    om_interval            interval_;
};

template<typename IGlobalize, typename IRecordFPArrayBinding, typename IQueryFPArrayBinding>
hierarchical_bind_map<IGlobalize, IRecordFPArrayBinding, IQueryFPArrayBinding>::
hierarchical_bind_map(IGlobalize& g,
                      IRecordFPArrayBinding& record_fp,
                      IQueryFPArrayBinding& query_fp,
                      om_interval interval)
    : globalizer_(g)
    , record_fp_(record_fp)
    , query_fp_(query_fp)
    , interval_(interval) {}

template<typename IGlobalize, typename IRecordFPArrayBinding, typename IQueryFPArrayBinding>
void hierarchical_bind_map<IGlobalize, IRecordFPArrayBinding, IQueryFPArrayBinding>::
bind(uint32_t global_key, framed_expr value) {
    DEBUG_ASSERT(
        !std::holds_alternative<expr::var>(value.skeleton->content)
        || global_key > globalizer_.globalize(
               value.frame_offset,
               std::get<expr::var>(value.skeleton->content).index));
    DEBUG_ASSERT(!query_fp_.query(interval_.open, global_key).has_value());
    record_fp_.record(interval_, global_key, value);
}

template<typename IGlobalize, typename IRecordFPArrayBinding, typename IQueryFPArrayBinding>
framed_expr hierarchical_bind_map<IGlobalize, IRecordFPArrayBinding, IQueryFPArrayBinding>::
whnf(framed_expr fe) {
    if (!std::holds_alternative<expr::var>(fe.skeleton->content))
        return fe;
    const uint32_t global_key = globalizer_.globalize(
        fe.frame_offset, std::get<expr::var>(fe.skeleton->content).index);
    const std::optional<framed_expr> result = query_fp_.query(interval_.open, global_key);
    if (!result)
        return fe;
    framed_expr resolved = whnf(*result);
    record_fp_.record(interval_, global_key, resolved);
    return resolved;
}

#endif
