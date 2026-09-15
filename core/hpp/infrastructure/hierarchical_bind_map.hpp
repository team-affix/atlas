#ifndef HIERARCHICAL_BIND_MAP_HPP
#define HIERARCHICAL_BIND_MAP_HPP

#include <optional>
#include "value_objects/framed_expr.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/om_label.hpp"

// hierarchical_bind_map: a node-scoped view over a fully_persistent_array that
// exposes the bind()/whnf() interface expected by the unifier.
//
// Constructed with the open/close om_labels of the current tree node, so
// callers (e.g. the unifier) never need to supply labels themselves.
//
// bind(global_key, value)
//   Records a binding in the underlying array for this node's interval.
//   Delegates directly to IRecordFPArrayBinding::record.
//
// whnf(fe)
//   Resolves fe to weak head normal form by following variable chains via
//   IQueryFPArrayBinding::query at this node's open label.  No path
//   compression: the persistent array is append-only after record().
//
// Template parameters — one per invoked method:
//   IGlobalize            — globalize(frame_offset, var_index) → uint32_t
//   IRecordFPArrayBinding — record(open, close, var_id, value)
//   IQueryFPArrayBinding  — query(open_label, var_id) → optional<framed_expr>

template<typename IGlobalize,
         typename IRecordFPArrayBinding,
         typename IQueryFPArrayBinding>
struct hierarchical_bind_map {
    hierarchical_bind_map(IGlobalize& g,
                          IRecordFPArrayBinding& record_fp,
                          IQueryFPArrayBinding& query_fp,
                          om_label open, om_label close);
    void bind(uint32_t global_key, framed_expr value);
    framed_expr whnf(framed_expr fe);
private:
    IGlobalize&            globalizer_;
    IRecordFPArrayBinding& record_fp_;
    IQueryFPArrayBinding&  query_fp_;
    om_label               open_;
    om_label               close_;
};

template<typename IGlobalize, typename IRecordFPArrayBinding, typename IQueryFPArrayBinding>
hierarchical_bind_map<IGlobalize, IRecordFPArrayBinding, IQueryFPArrayBinding>::
hierarchical_bind_map(IGlobalize& g,
                      IRecordFPArrayBinding& record_fp,
                      IQueryFPArrayBinding& query_fp,
                      om_label open, om_label close)
    : globalizer_(g)
    , record_fp_(record_fp)
    , query_fp_(query_fp)
    , open_(open)
    , close_(close) {}

template<typename IGlobalize, typename IRecordFPArrayBinding, typename IQueryFPArrayBinding>
void hierarchical_bind_map<IGlobalize, IRecordFPArrayBinding, IQueryFPArrayBinding>::
bind(uint32_t global_key, framed_expr value) {
    record_fp_.record(open_, close_, global_key, value);
}

template<typename IGlobalize, typename IRecordFPArrayBinding, typename IQueryFPArrayBinding>
framed_expr hierarchical_bind_map<IGlobalize, IRecordFPArrayBinding, IQueryFPArrayBinding>::
whnf(framed_expr fe) {
    if (!std::holds_alternative<expr::var>(fe.skeleton->content))
        return fe;
    const uint32_t global_key = globalizer_.globalize(
        fe.frame_offset, std::get<expr::var>(fe.skeleton->content).index);
    const std::optional<framed_expr> result = query_fp_.query(open_, global_key);
    if (!result)
        return fe;
    return whnf(*result);
}

#endif
