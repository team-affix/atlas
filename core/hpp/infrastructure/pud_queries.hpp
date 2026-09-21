#ifndef PUD_QUERIES_HPP
#define PUD_QUERIES_HPP

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_unfold_site.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "debug_assert.hpp"

template<typename IGetAddedBodyGoals,
         typename IGetLvc,
         typename IResumeCandidateSearch,
         typename IResumeWitnessSearch>
struct pud_queries {
    pud_queries(IGetAddedBodyGoals& get_added_body_goals,
                IGetLvc& get_lvc,
                IResumeCandidateSearch& resume_candidate_search,
                IResumeWitnessSearch& resume_witness_search);
    void adopt_axiom(const pud_rule_id* axiom);
    pud_unfold_site unfold_site(const pud_rule_id* leaf, size_t body_goal_idx);
    std::vector<pud_forced_unfold> replace_unfolded(const pud_rule_id* leaf,
                                                    size_t body_goal_idx,
                                                    const std::vector<const pud_rule_id*>& children);
private:
    using group_t = std::vector<std::unique_ptr<pud_candidate_search_context>>;
    using groups_t = std::vector<group_t>;
    using group_ptrs_t = std::vector<pud_candidate_search_context*>;
    using groups_ptrs_t = std::vector<group_ptrs_t>;
    using group_values_t = std::vector<pud_candidate_search_context>;
    using groups_values_t = std::vector<group_values_t>;
    using context_set_t = std::unordered_set<pud_candidate_search_context*>;
    using witness_set_t = std::unordered_set<const pud_rule_id*>;
    using leaf_set_t = std::unordered_set<const pud_rule_id*>;
    struct rule_id_less {
        bool operator()(const pud_rule_id* a, const pud_rule_id* b) const;
    };

    void ensure_booted();
    void boot();
    void install(const pud_rule_id* leaf);
    void fork_child(const pud_rule_id* child,
                    const groups_values_t& leftover_templates);
    void clear(const pud_rule_id* leaf);
    void invalidate_leaf(const pud_rule_id* node);
    void commit_groups(const pud_rule_id* leaf, groups_values_t groups);
    void replace_owned(const pud_rule_id* leaf, groups_values_t groups);
    void resume_context(pud_candidate_search_context& ctx);
    void watch_context(pud_candidate_search_context& ctx);
    void unwatch_context(pud_candidate_search_context* ctx);
    void watch(const pud_rule_id* witness, pud_candidate_search_context* ctx);
    void mark_dirty(const pud_rule_id* leaf);
    void resume_dead_witness(pud_candidate_search_context& ctx, const pud_rule_id* node);
    std::vector<pud_candidate_search_context*> live_contexts(group_ptrs_t& group);
    group_values_t root_group(const pud_rule_id* query_leaf,
                              size_t body_goal_idx,
                              const expr* body_goal,
                              uint32_t frame_offset);
    std::vector<const pud_rule_id*> take_dirty_leaves();
    const groups_ptrs_t& get(const pud_rule_id* leaf);
    std::vector<pud_forced_unfold> take_forced_unfolds();

    IGetAddedBodyGoals& get_added_body_goals_;
    IGetLvc& get_lvc_;
    IResumeCandidateSearch& resume_candidate_search_;
    IResumeWitnessSearch& resume_witness_search_;
    std::vector<const pud_rule_id*> axioms_;
    std::unordered_map<const pud_rule_id*, groups_t> owned_;
    std::unordered_map<const pud_rule_id*, groups_ptrs_t> ptrs_;
    std::unordered_map<pud_candidate_search_context*, const pud_rule_id*> context_leaf_;
    std::unordered_map<const pud_rule_id*, context_set_t> by_witness_;
    std::unordered_map<pud_candidate_search_context*, witness_set_t> by_context_;
    leaf_set_t dirty_leaves_;
    bool booted_;
};

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
bool pud_queries<IGABG, IGL, IRCS, IRWS>::rule_id_less::operator()(
        const pud_rule_id* a, const pud_rule_id* b) const {
    return *a < *b;
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
pud_queries<IGABG, IGL, IRCS, IRWS>::pud_queries(
        IGABG& get_added_body_goals,
        IGL& get_lvc,
        IRCS& resume_candidate_search,
        IRWS& resume_witness_search)
    : get_added_body_goals_(get_added_body_goals)
    , get_lvc_(get_lvc)
    , resume_candidate_search_(resume_candidate_search)
    , resume_witness_search_(resume_witness_search)
    , axioms_()
    , owned_()
    , ptrs_()
    , context_leaf_()
    , by_witness_()
    , by_context_()
    , dirty_leaves_()
    , booted_(false) {}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
typename pud_queries<IGABG, IGL, IRCS, IRWS>::group_values_t
pud_queries<IGABG, IGL, IRCS, IRWS>::root_group(
        const pud_rule_id* query_leaf,
        size_t body_goal_idx,
        const expr* body_goal,
        uint32_t frame_offset) {
    group_values_t group;
    for (const pud_rule_id* axiom : axioms_) {
        group.push_back(pud_candidate_search_context{
            query_leaf,
            body_goal_idx,
            body_goal,
            frame_offset,
            axiom,
            std::nullopt,
            get_added_body_goals_.get(axiom)});
    }
    return group;
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::replace_owned(
        const pud_rule_id* leaf, groups_values_t groups) {
    DEBUG_ASSERT(!owned_.contains(leaf));
    groups_t owned;
    groups_ptrs_t ptrs;
    owned.reserve(groups.size());
    ptrs.reserve(groups.size());
    for (group_values_t& group : groups) {
        group_t owned_group;
        group_ptrs_t ptr_group;
        owned_group.reserve(group.size());
        ptr_group.reserve(group.size());
        for (pud_candidate_search_context& ctx : group) {
            owned_group.push_back(
                std::make_unique<pud_candidate_search_context>(std::move(ctx)));
            pud_candidate_search_context* ptr = owned_group.back().get();
            ptr_group.push_back(ptr);
            context_leaf_[ptr] = leaf;
        }
        owned.push_back(std::move(owned_group));
        ptrs.push_back(std::move(ptr_group));
    }
    owned_[leaf] = std::move(owned);
    ptrs_[leaf] = std::move(ptrs);
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::watch(
        const pud_rule_id* witness, pud_candidate_search_context* ctx) {
    by_witness_[witness].insert(ctx);
    by_context_[ctx].insert(witness);
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::watch_context(
        pud_candidate_search_context& ctx) {
    if (ctx.witnesses.has_value()) {
        DEBUG_ASSERT(ctx.witnesses->a.current != nullptr);
        DEBUG_ASSERT(ctx.witnesses->b.current != nullptr);
        watch(ctx.witnesses->a.current, &ctx);
        watch(ctx.witnesses->b.current, &ctx);
        return;
    }
    if (ctx.cursor == nullptr)
        return;
    watch(ctx.cursor, &ctx);
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::unwatch_context(
        pud_candidate_search_context* ctx) {
    auto ctx_it = by_context_.find(ctx);
    if (ctx_it == by_context_.end())
        return;
    const witness_set_t witnesses = ctx_it->second;
    by_context_.erase(ctx_it);
    for (const pud_rule_id* witness : witnesses) {
        auto witness_it = by_witness_.find(witness);
        if (witness_it == by_witness_.end())
            continue;
        witness_it->second.erase(ctx);
        if (witness_it->second.empty())
            by_witness_.erase(witness_it);
    }
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::resume_context(
        pud_candidate_search_context& ctx) {
    resume_candidate_search_.resume(ctx);
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::mark_dirty(const pud_rule_id* leaf) {
    dirty_leaves_.insert(leaf);
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::commit_groups(
        const pud_rule_id* leaf, groups_values_t groups) {
    replace_owned(leaf, std::move(groups));
    for (group_ptrs_t& group : ptrs_.at(leaf)) {
        for (pud_candidate_search_context* ctx : group) {
            resume_context(*ctx);
            watch_context(*ctx);
        }
    }
    mark_dirty(leaf);
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::install(const pud_rule_id* leaf) {
    const uint32_t frame_offset = get_lvc_.get(leaf);
    groups_values_t groups;
    size_t body_goal_idx = 0;
    for (const expr* goal : get_added_body_goals_.get(leaf)) {
        groups.push_back(root_group(leaf, body_goal_idx, goal, frame_offset));
        ++body_goal_idx;
    }
    commit_groups(leaf, std::move(groups));
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::boot() {
    for (const pud_rule_id* axiom : axioms_)
        install(axiom);
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::ensure_booted() {
    if (booted_)
        return;
    booted_ = true;
    boot();
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::adopt_axiom(
        const pud_rule_id* axiom) {
    DEBUG_ASSERT(!booted_);
    axioms_.push_back(axiom);
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::fork_child(
        const pud_rule_id* child,
        const groups_values_t& leftover_templates) {
    const uint32_t child_lvc = get_lvc_.get(child);
    groups_values_t child_groups;
    size_t body_goal_idx = 0;
    for (const group_values_t& leftover : leftover_templates) {
        group_values_t forked;
        for (pud_candidate_search_context ctx : leftover) {
            ctx.query_leaf = child;
            ctx.body_goal_idx = body_goal_idx;
            ctx.frame_offset = child_lvc;
            if (ctx.witnesses.has_value()) {
                ctx.witnesses->a.query_leaf = child;
                ctx.witnesses->a.body_goal_idx = body_goal_idx;
                ctx.witnesses->a.frame_offset = child_lvc;
                ctx.witnesses->b.query_leaf = child;
                ctx.witnesses->b.body_goal_idx = body_goal_idx;
                ctx.witnesses->b.frame_offset = child_lvc;
            }
            forked.push_back(std::move(ctx));
        }
        child_groups.push_back(std::move(forked));
        ++body_goal_idx;
    }
    for (const expr* goal : get_added_body_goals_.get(child)) {
        child_groups.push_back(root_group(child, body_goal_idx, goal, child_lvc));
        ++body_goal_idx;
    }
    commit_groups(child, std::move(child_groups));
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::clear(const pud_rule_id* leaf) {
    auto ptrs_it = ptrs_.find(leaf);
    if (ptrs_it != ptrs_.end()) {
        for (group_ptrs_t& group : ptrs_it->second) {
            for (pud_candidate_search_context* ctx : group) {
                unwatch_context(ctx);
                context_leaf_.erase(ctx);
            }
        }
    }
    owned_.erase(leaf);
    ptrs_.erase(leaf);
    dirty_leaves_.erase(leaf);
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
const typename pud_queries<IGABG, IGL, IRCS, IRWS>::groups_ptrs_t&
pud_queries<IGABG, IGL, IRCS, IRWS>::get(const pud_rule_id* leaf) {
    ensure_booted();
    return ptrs_.at(leaf);
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::resume_dead_witness(
        pud_candidate_search_context& ctx, const pud_rule_id* node) {
    if (!ctx.witnesses.has_value()) {
        if (ctx.cursor == node)
            resume_candidate_search_.resume(ctx);
        return;
    }
    pud_witness_search_context* side = nullptr;
    if (ctx.witnesses->a.current == node)
        side = &ctx.witnesses->a;
    else if (ctx.witnesses->b.current == node)
        side = &ctx.witnesses->b;
    if (side == nullptr)
        return;
    resume_witness_search_.resume(*side);
    if (side->current == nullptr)
        resume_candidate_search_.resume(ctx);
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
void pud_queries<IGABG, IGL, IRCS, IRWS>::invalidate_leaf(
        const pud_rule_id* node) {
    auto witness_it = by_witness_.find(node);
    if (witness_it == by_witness_.end())
        return;
    const std::vector<pud_candidate_search_context*> contexts(
        witness_it->second.begin(), witness_it->second.end());
    for (pud_candidate_search_context* ctx : contexts) {
        unwatch_context(ctx);
        resume_dead_witness(*ctx, node);
        watch_context(*ctx);
        auto leaf_it = context_leaf_.find(ctx);
        if (leaf_it != context_leaf_.end())
            mark_dirty(leaf_it->second);
    }
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
std::vector<const pud_rule_id*>
pud_queries<IGABG, IGL, IRCS, IRWS>::take_dirty_leaves() {
    std::vector<const pud_rule_id*> out(dirty_leaves_.begin(), dirty_leaves_.end());
    std::sort(out.begin(), out.end(), rule_id_less{});
    dirty_leaves_.clear();
    return out;
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
std::vector<pud_candidate_search_context*>
pud_queries<IGABG, IGL, IRCS, IRWS>::live_contexts(group_ptrs_t& group) {
    std::vector<pud_candidate_search_context*> live;
    for (pud_candidate_search_context* ctx : group) {
        resume_candidate_search_.resume(*ctx);
        if (ctx->cursor == nullptr)
            continue;
        live.push_back(ctx);
    }
    return live;
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
pud_unfold_site
pud_queries<IGABG, IGL, IRCS, IRWS>::unfold_site(
        const pud_rule_id* leaf, size_t body_goal_idx) {
    const groups_ptrs_t& groups = get(leaf);
    DEBUG_ASSERT(body_goal_idx < groups.size());
    group_ptrs_t group = groups[body_goal_idx];
    DEBUG_ASSERT(!group.empty());
    return pud_unfold_site{group[0]->body_goal, live_contexts(group)};
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
std::vector<pud_forced_unfold>
pud_queries<IGABG, IGL, IRCS, IRWS>::replace_unfolded(
        const pud_rule_id* leaf,
        size_t body_goal_idx,
        const std::vector<const pud_rule_id*>& children) {
    const groups_ptrs_t& parent_groups = get(leaf);
    DEBUG_ASSERT(body_goal_idx < parent_groups.size());
    groups_values_t leftover_templates;
    for (size_t idx = 0; idx < parent_groups.size(); ++idx) {
        if (idx == body_goal_idx)
            continue;
        group_values_t copied;
        for (pud_candidate_search_context* ctx : parent_groups[idx])
            copied.push_back(*ctx);
        leftover_templates.push_back(std::move(copied));
    }
    clear(leaf);
    for (const pud_rule_id* child : children)
        fork_child(child, leftover_templates);
    invalidate_leaf(leaf);
    return take_forced_unfolds();
}

template<typename IGABG, typename IGL, typename IRCS, typename IRWS>
std::vector<pud_forced_unfold>
pud_queries<IGABG, IGL, IRCS, IRWS>::take_forced_unfolds() {
    ensure_booted();
    std::vector<pud_forced_unfold> yields;
    const std::vector<const pud_rule_id*> dirty = take_dirty_leaves();
    for (const pud_rule_id* leaf : dirty) {
        const groups_ptrs_t& groups = get(leaf);
        bool refuted = false;
        std::vector<size_t> unit_idxs;
        for (size_t idx = 0; idx < groups.size(); ++idx) {
            group_ptrs_t group = groups[idx];
            const size_t live = live_contexts(group).size();
            if (live == 0) {
                refuted = true;
                break;
            }
            if (live == 1)
                unit_idxs.push_back(idx);
        }
        if (refuted) {
            yields.push_back(pud_forced_unfold{pud_forced_unfold::refuted{leaf}});
            continue;
        }
        for (size_t idx : unit_idxs)
            yields.push_back(pud_forced_unfold{pud_forced_unfold::unit{leaf, idx}});
    }
    return yields;
}

#endif
