#ifndef REPRESENTATIVE_CHANGE_LOGGER_STACK_HPP
#define REPRESENTATIVE_CHANGE_LOGGER_STACK_HPP

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <variant>
#include <vector>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_normalizer.hpp"
#include "infrastructure/pud_unifier.hpp"
#include "infrastructure/representative_change_logger.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "pud_unifier_bind_map_stack.hpp"
#include "ubs_bind_map_test_maps.hpp"
#include "value_objects/framed_expr.hpp"

struct traced_changed_rep_sink {
    std::vector<uint32_t> changed_reps;
    std::vector<bool> was_instantiate;
    bool next_push_is_instantiate_;

    traced_changed_rep_sink()
        : next_push_is_instantiate_(false) {}

    void mark_next_equate() {
        next_push_is_instantiate_ = false;
    }

    void mark_next_instantiate() {
        next_push_is_instantiate_ = true;
    }

    void push_back(uint32_t rep) {
        changed_reps.push_back(rep);
        was_instantiate.push_back(next_push_is_instantiate_);
    }

    void clear() {
        changed_reps.clear();
        was_instantiate.clear();
    }
};

struct logged_ubs_bind_map_stack {
    ubs_bind_map_expr_pool_backing bind_backing;
    ubs_bind_map_expr_pool_backing::bind_map_t bind_map{bind_backing.make_bind_map()};
    traced_changed_rep_sink sink;
    using rep_logger_t = representative_change_logger<
        ubs_bind_map_expr_pool_backing::bind_map_t,
        ubs_bind_map_expr_pool_backing::bind_map_t,
        traced_changed_rep_sink>;
    rep_logger_t rep_logger;

    logged_ubs_bind_map_stack()
        : rep_logger(bind_map, bind_map, sink) {}
};

template<typename IRepLogger>
struct logged_equate_instantiate_gate {
    logged_equate_instantiate_gate(IRepLogger& rep_logger, traced_changed_rep_sink& sink)
        : rep_logger_(rep_logger)
        , sink_(sink) {}

    void equate(uint32_t rep_a, uint32_t rep_b) {
        sink_.mark_next_equate();
        rep_logger_.equate(rep_a, rep_b);
    }

    void instantiate(uint32_t rep, uint32_t functor_id, const std::vector<const expr*>& args) {
        sink_.mark_next_instantiate();
        rep_logger_.instantiate(rep, functor_id, args);
    }

private:
    IRepLogger& rep_logger_;
    traced_changed_rep_sink& sink_;
};

struct pud_logged_unifier_bind_map_stack {
    globalizer globalize;
    ubs_bind_map_expr_pool_backing bind_backing;
    ubs_bind_map_expr_pool_backing::bind_map_t bind_map{bind_backing.make_bind_map()};
    traced_changed_rep_sink sink;
    using rep_logger_t = representative_change_logger<
        ubs_bind_map_expr_pool_backing::bind_map_t,
        ubs_bind_map_expr_pool_backing::bind_map_t,
        traced_changed_rep_sink>;
    rep_logger_t rep_logger;
    logged_equate_instantiate_gate<rep_logger_t> bind_gate;
    global_normalizer<globalizer, expr_pool, ubs_bind_map_expr_pool_backing::bind_map_t> normalizer{
        globalize, bind_backing.pool, bind_map};

    using pud_unifier_t = pud_unifier<
        globalizer,
        global_normalizer<globalizer, expr_pool, ubs_bind_map_expr_pool_backing::bind_map_t>,
        ubs_bind_map_expr_pool_backing::bind_map_t,
        logged_equate_instantiate_gate<rep_logger_t>,
        logged_equate_instantiate_gate<rep_logger_t>>;

    pud_unifier_t framed_unifier;

    pud_logged_unifier_bind_map_stack()
        : rep_logger(bind_map, bind_map, sink)
        , bind_gate(rep_logger, sink)
        , framed_unifier(globalize, normalizer, bind_map, bind_gate, bind_gate) {}
};

inline uint32_t logged_global_rep_index(
    ubs_bind_map_expr_pool_backing::bind_map_t& bind_map,
    uint32_t var) {
    return ubs_rep_index(bind_map.resolve(var));
}

inline uint32_t logged_global_rep_index(
    globalizer& globalize,
    ubs_bind_map_expr_pool_backing::bind_map_t& bind_map,
    uint32_t frame_offset,
    uint32_t local_index) {
    return logged_global_rep_index(bind_map, globalize.globalize(frame_offset, local_index));
}

struct rep_change_trace {
    std::vector<uint32_t> reps;
    std::vector<bool> was_instantiate;
};

inline rep_change_trace trace_from_sink(const traced_changed_rep_sink& sink) {
    rep_change_trace trace;
    trace.reps = sink.changed_reps;
    trace.was_instantiate = sink.was_instantiate;
    return trace;
}

inline rep_change_trace dedupe_rep_change_trace(const rep_change_trace& full) {
    std::unordered_map<uint32_t, size_t> last_index_for_rep;
    for (size_t step = 0; step < full.reps.size(); ++step)
        last_index_for_rep[full.reps[step]] = step;
    std::vector<size_t> ordered_steps;
    for (const auto& entry : last_index_for_rep)
        ordered_steps.push_back(entry.second);
    std::sort(ordered_steps.begin(), ordered_steps.end());
    rep_change_trace deduped;
    for (size_t step : ordered_steps) {
        deduped.reps.push_back(full.reps[step]);
        deduped.was_instantiate.push_back(full.was_instantiate[step]);
    }
    return deduped;
}

inline bool logged_expr_bindings_equal(const expr* left, const expr* right) {
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
        if (!logged_expr_bindings_equal(left_functor.args[arg], right_functor.args[arg]))
            return false;
    }
    return true;
}

inline void replay_rep_changes(
    logged_ubs_bind_map_stack& fresh,
    ubs_bind_map_expr_pool_backing::bind_map_t& original_bind_map,
    const rep_change_trace& trace) {
    for (size_t step = 0; step < trace.reps.size(); ++step) {
        const uint32_t rep = trace.reps[step];
        const expr* final_value = original_bind_map.resolve(rep);
        if (std::holds_alternative<expr::functor>(final_value->content)) {
            const expr::functor& bound_functor = std::get<expr::functor>(final_value->content);
            fresh.rep_logger.instantiate(rep, bound_functor.id, bound_functor.args);
            continue;
        }
        const uint32_t merged_to = ubs_rep_index(final_value);
        if (rep != merged_to)
            fresh.rep_logger.equate(rep, merged_to);
    }
}

inline bool resolve_maps_match(
    ubs_bind_map_expr_pool_backing::bind_map_t& left,
    ubs_bind_map_expr_pool_backing::bind_map_t& right,
    uint32_t var_count) {
    for (uint32_t var = 0; var < var_count; ++var) {
        if (!logged_expr_bindings_equal(left.resolve(var), right.resolve(var)))
            return false;
    }
    return true;
}

#endif
