#ifndef PUD_STRUCTURE_WATCH_DESCENDER_HPP
#define PUD_STRUCTURE_WATCH_DESCENDER_HPP

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <variant>
#include <vector>
#include <immer/set.hpp>
#include "infrastructure/coroutine.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_mhsw_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_specialization.hpp"
#include "value_objects/pud_structure_watch_descent.hpp"
#include "debug_assert.hpp"

// Invariant: every structure set (head_to_reps) holds only representatives of unbound classes.
template<typename IGetNodeAddedSpecializations, typename IResolve>
struct pud_structure_watch_descender {
    pud_structure_watch_descender(IGetNodeAddedSpecializations& get_node_added_specializations);
    pud_structure_watch_descent root_descent(pud_node_id root);
    pud_structure_watch_descent watch(
        const pud_structure_watch_descent& descent,
        IResolve& resolve,
        const expr* watched);
    pud_structure_watch_descent unwatch(
        const pud_structure_watch_descent& descent,
        pud_mhsw_head_id head_id);
    coroutine<pud_mhsw_head_id, pud_structure_watch_descent> descend(
        pud_structure_watch_descent descent,
        IResolve& resolve,
        pud_node_id child);
private:
    using head_set_t = immer::set<pud_mhsw_head_id>;
    using rep_set_t = immer::set<uint32_t>;

    std::vector<pud_mhsw_head_id> propagate_rep_updated(
        pud_structure_watch_descent& descent,
        uint32_t rep_a,
        uint32_t rep_b);
    std::vector<pud_mhsw_head_id> propagate_rep_instantiated(
        pud_structure_watch_descent& descent,
        IResolve& resolve,
        uint32_t rep,
        const expr* value);
    void visit_reps(IResolve& resolve, const expr* visited, rep_set_t& reps);
    bool holds_only_reps(const pud_structure_watch_descent& descent, IResolve& resolve);
    head_set_t find_heads(const pud_structure_watch_descent& descent, uint32_t rep);
    rep_set_t find_reps(const pud_structure_watch_descent& descent, pud_mhsw_head_id head_id);
    void link(pud_structure_watch_descent& descent, uint32_t rep, pud_mhsw_head_id head_id);
    void unlink(pud_structure_watch_descent& descent, uint32_t rep, pud_mhsw_head_id head_id);

    IGetNodeAddedSpecializations& get_node_added_specializations_;
};

#define PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE                                             \
    template<typename IGetNodeAddedSpecializations, typename IResolve>
#define PUD_STRUCTURE_WATCH_DESCENDER                                                      \
    pud_structure_watch_descender<IGetNodeAddedSpecializations, IResolve>

PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE
PUD_STRUCTURE_WATCH_DESCENDER::pud_structure_watch_descender(
    IGetNodeAddedSpecializations& get_node_added_specializations)
    : get_node_added_specializations_(get_node_added_specializations) {}

PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE
pud_structure_watch_descent PUD_STRUCTURE_WATCH_DESCENDER::root_descent(pud_node_id root) {
    return pud_structure_watch_descent{.next_head_id = 0, .rep_to_heads = {}, .head_to_reps = {}};
}

PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE
pud_structure_watch_descent PUD_STRUCTURE_WATCH_DESCENDER::watch(
    const pud_structure_watch_descent& descent,
    IResolve& resolve,
    const expr* watched) {
    rep_set_t reps;
    visit_reps(resolve, watched, reps);
    pud_structure_watch_descent result = descent;
    const pud_mhsw_head_id head_id = result.next_head_id++;
    result.head_to_reps = result.head_to_reps.set(head_id, reps);
    for (uint32_t rep : reps)
        link(result, rep, head_id);
    DEBUG_ASSERT(holds_only_reps(result, resolve));
    return result;
}

PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE
pud_structure_watch_descent PUD_STRUCTURE_WATCH_DESCENDER::unwatch(
    const pud_structure_watch_descent& descent,
    pud_mhsw_head_id head_id) {
    pud_structure_watch_descent result = descent;
    for (uint32_t rep : find_reps(result, head_id))
        unlink(result, rep, head_id);
    result.head_to_reps = result.head_to_reps.erase(head_id);
    return result;
}

PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE
coroutine<pud_mhsw_head_id, pud_structure_watch_descent> PUD_STRUCTURE_WATCH_DESCENDER::descend(
    pud_structure_watch_descent descent,
    IResolve& resolve,
    pud_node_id child) {
    std::unordered_set<pud_mhsw_head_id> yielded;
    for (const pud_specialization& specialization : get_node_added_specializations_.get(child)) {
        const expr::var* var = std::get_if<expr::var>(&specialization.value->content);
        const std::vector<pud_mhsw_head_id> changed = var
            ? propagate_rep_updated(descent, specialization.var_idx, var->index)
            : propagate_rep_instantiated(descent, resolve, specialization.var_idx, specialization.value);
        for (pud_mhsw_head_id head_id : changed) {
            const bool first_change = yielded.insert(head_id).second;
            if (!first_change)
                continue;
            co_yield head_id;
        }
    }
    DEBUG_ASSERT(holds_only_reps(descent, resolve));
    co_return descent;
}

// Both reps are representatives of distinct classes; the merged class keeps the smaller index.
PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE
std::vector<pud_mhsw_head_id> PUD_STRUCTURE_WATCH_DESCENDER::propagate_rep_updated(
    pud_structure_watch_descent& descent,
    uint32_t rep_a,
    uint32_t rep_b) {
    const uint32_t surviving_rep = std::min(rep_a, rep_b);
    const uint32_t demoted_rep = std::max(rep_a, rep_b);
    std::vector<pud_mhsw_head_id> changed;
    for (pud_mhsw_head_id head_id : find_heads(descent, demoted_rep)) {
        const rep_set_t reps = find_reps(descent, head_id);
        const bool collapsed = reps.count(surviving_rep) != 0;
        descent.head_to_reps = descent.head_to_reps.set(head_id, reps.erase(demoted_rep).insert(surviving_rep));
        link(descent, surviving_rep, head_id);
        if (!collapsed)
            continue;
        changed.push_back(head_id);
    }
    descent.rep_to_heads = descent.rep_to_heads.erase(demoted_rep);
    return changed;
}

PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE
std::vector<pud_mhsw_head_id> PUD_STRUCTURE_WATCH_DESCENDER::propagate_rep_instantiated(
    pud_structure_watch_descent& descent,
    IResolve& resolve,
    uint32_t rep,
    const expr* value) {
    rep_set_t introduced_reps;
    visit_reps(resolve, value, introduced_reps);
    std::vector<pud_mhsw_head_id> changed;
    for (pud_mhsw_head_id head_id : find_heads(descent, rep)) {
        rep_set_t reps = find_reps(descent, head_id).erase(rep);
        for (uint32_t introduced_rep : introduced_reps) {
            reps = reps.insert(introduced_rep);
            link(descent, introduced_rep, head_id);
        }
        descent.head_to_reps = descent.head_to_reps.set(head_id, reps);
        changed.push_back(head_id);
    }
    descent.rep_to_heads = descent.rep_to_heads.erase(rep);
    return changed;
}

PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE
void PUD_STRUCTURE_WATCH_DESCENDER::visit_reps(IResolve& resolve, const expr* visited, rep_set_t& reps) {
    if (const expr::var* var = std::get_if<expr::var>(&visited->content)) {
        const expr* resolved = resolve.resolve(var->index);
        if (const expr::var* rep = std::get_if<expr::var>(&resolved->content)) {
            reps = reps.insert(rep->index);
            return;
        }
        visit_reps(resolve, resolved, reps);
        return;
    }
    for (const expr* arg : std::get<expr::functor>(visited->content).args)
        visit_reps(resolve, arg, reps);
}

PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE
bool PUD_STRUCTURE_WATCH_DESCENDER::holds_only_reps(
    const pud_structure_watch_descent& descent,
    IResolve& resolve) {
    for (const auto& [head_id, reps] : descent.head_to_reps) {
        for (uint32_t rep : reps) {
            const expr::var* resolved = std::get_if<expr::var>(&resolve.resolve(rep)->content);
            const bool is_rep = resolved && resolved->index == rep;
            if (!is_rep)
                return false;
        }
    }
    return true;
}

PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE
typename PUD_STRUCTURE_WATCH_DESCENDER::head_set_t PUD_STRUCTURE_WATCH_DESCENDER::find_heads(
    const pud_structure_watch_descent& descent,
    uint32_t rep) {
    const head_set_t* heads = descent.rep_to_heads.find(rep);
    if (!heads)
        return {};
    return *heads;
}

PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE
typename PUD_STRUCTURE_WATCH_DESCENDER::rep_set_t PUD_STRUCTURE_WATCH_DESCENDER::find_reps(
    const pud_structure_watch_descent& descent,
    pud_mhsw_head_id head_id) {
    const rep_set_t* reps = descent.head_to_reps.find(head_id);
    if (!reps)
        return {};
    return *reps;
}

PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE
void PUD_STRUCTURE_WATCH_DESCENDER::link(
    pud_structure_watch_descent& descent,
    uint32_t rep,
    pud_mhsw_head_id head_id) {
    descent.rep_to_heads = descent.rep_to_heads.set(rep, find_heads(descent, rep).insert(head_id));
}

PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE
void PUD_STRUCTURE_WATCH_DESCENDER::unlink(
    pud_structure_watch_descent& descent,
    uint32_t rep,
    pud_mhsw_head_id head_id) {
    const head_set_t heads = find_heads(descent, rep).erase(head_id);
    if (heads.empty()) {
        descent.rep_to_heads = descent.rep_to_heads.erase(rep);
        return;
    }
    descent.rep_to_heads = descent.rep_to_heads.set(rep, heads);
}

#undef PUD_STRUCTURE_WATCH_DESCENDER
#undef PUD_STRUCTURE_WATCH_DESCENDER_TEMPLATE

#endif
