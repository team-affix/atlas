#ifndef DBUCT_CDCL_ELIMINATION_GENERATOR_HPP
#define DBUCT_CDCL_ELIMINATION_GENERATOR_HPP

#include <algorithm>
#include <cstddef>
#include <deque>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stack>
#include <list>
#include "infrastructure/coroutine.hpp"
#include "value_objects/avoidance.hpp"
#include "value_objects/avoidance_id.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/avoidance_action.hpp"
#include "value_objects/raised_unit_avoidance.hpp"
#include "debug_assert.hpp"

// NOTES: learn() can be void since learn() is always expected to be called immediately before pop(),
// since learn() is only ever invoked at a terminal state where there is nothing left to do.
template<typename ITryGetChosenGoalCandidate,
    typename IGetPenultimateMctsFrameDepth,
    typename IDeriveDecisionLemma,
    typename IGetUltimateDecision,
    typename IGetPenultimateDecision>
struct dbuct_cdcl_elimination_generator {
    dbuct_cdcl_elimination_generator(
        ITryGetChosenGoalCandidate&,
        IGetPenultimateMctsFrameDepth&,
        IDeriveDecisionLemma&,
        IGetUltimateDecision&,
        IGetPenultimateDecision&);
    void learn();
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
    void push_frame();
    coroutine<const resolution_lineage*, void> pop_frame();
private:
    size_t scan(const avoidance& av) const;
    std::optional<const resolution_lineage*> visit_avoidance(avoidance_id, const resolution_lineage*);
    void undo_action(const avoidance_action& action);
    void link_watchers(avoidance_id id);

    std::unordered_map<avoidance_id, avoidance> avoidances_;
    size_t next_avoidance_id_;
    std::unordered_map<const goal_lineage*, std::unordered_set<avoidance_id>> watched_goals_;

    // frame-based system for dbuct
    // keeps track of what happened at each frame, so we can roll it back and make adjustments.
    // for all avoidances, we do the following algorithm as we slide leftward the visitation boundary:
    // [a,b,c,d|(h),(f),g,e]
    // [a,b,c|(f),(h),d,g,e]
    // [a,b|(h),(f),c,d,g,e]
    // [a|(f),(h),b,c,d,g,e]
    // [(h),(f),a,b,c,d,g,e]
    // Basically, h and f were the watchers at the end of the episode, and since even at the end of the
    // episode they are not firing, we consider them "good choices" to be the watchers in the future, so
    // we bubble them up to the front of the list (never violating the visitation boundary, we never want
    // to modify the list to the left of the pipe). The pipe slides leftward as we backtrack basically.
    struct frame {
        std::list<avoidance_action> actions_;
        std::list<raised_unit_avoidance> raised_unit_avoidance_lump;
    };
    std::stack<frame> frame_stack_;
    
    ITryGetChosenGoalCandidate& try_get_chosen_goal_candidate_;
    IGetPenultimateMctsFrameDepth& get_penultimate_mcts_frame_depth_;
    IDeriveDecisionLemma& derive_decision_lemma_;
    IGetUltimateDecision& get_ultimate_decision_;
    IGetPenultimateDecision& get_penultimate_decision_;
};

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD>
dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD>::dbuct_cdcl_elimination_generator(
    ITGCC& tgcc, IGUB& gub, IDL& dl, IGUD& gud, IGPD& gpd)
    : next_avoidance_id_(0), frame_stack_(std::deque<frame>{frame{}}),
      try_get_chosen_goal_candidate_(tgcc), get_penultimate_mcts_frame_depth_(gub),
      derive_decision_lemma_(dl), get_ultimate_decision_(gud),
      get_penultimate_decision_(gpd) {}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD>
void
dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD>::learn() {
    // get the decision lemma from the current position
    lemma l = derive_decision_lemma_.derive_decision_lemma();
    
    auto resolutions = l.get_resolutions();
    
    if (resolutions.size() == 0)
        return;

    const avoidance_id id = next_avoidance_id_++;

    // raise the unit avoidance
    size_t unit_boundary = get_penultimate_mcts_frame_depth_.get_penultimate_mcts_frame_depth();
    frame_stack_.top().raised_unit_avoidance_lump.emplace_back(raised_unit_avoidance{id, unit_boundary});

    // if size==1, store a degenerate single-member avoidance so pop_frame can find
    // the resolution to float to the top. watcher_b_pos is a SIZE_MAX poison: a unit
    // has no second literal to watch. A unit's unit_boundary is 0, so it can never
    // hit pop_frame's arm branch (frame_stack_.size() < 0 is impossible), which is
    // the only site that would dereference watcher_b_pos -- see the assert there.
    if (resolutions.size() == 1) {
        avoidances_.emplace(id, avoidance{{*resolutions.begin()}, 0, SIZE_MAX});
        return;
    }

    // get the ultimate and penultimate decisions
    auto ultimate_decision = get_ultimate_decision_.get_ultimate_decision();
    auto penultimate_decision = get_penultimate_decision_.get_penultimate_decision();

    // remove the decisions from the avoidance set
    resolutions.erase(ultimate_decision);
    resolutions.erase(penultimate_decision);
    
    // create the avoidance members
    std::vector<const resolution_lineage*> members(2 + resolutions.size());
    members[0] = ultimate_decision;
    members[1] = penultimate_decision;
    std::copy(resolutions.begin(), resolutions.end(), members.begin() + 2);
    
    avoidances_.emplace(id, avoidance{std::move(members), 0, 1});

    // we don't link the goals here since avoidances are always non-dormant at creation,
    // so we don't need to watch the goals.
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD>
coroutine<const resolution_lineage*, void>
dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD>::constrain(const resolution_lineage* rl) {
    const auto it = watched_goals_.find(rl->parent);
    if (it == watched_goals_.end())
        co_return;

    for (auto i = it->second.begin(); i != it->second.end(); ) {
        const avoidance_id id = *i;
        it->second.erase(i++);
        if (auto forced = visit_avoidance(id, rl))
            co_yield *forced;
    }
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD>
size_t dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD>::scan(const avoidance& av) const {
    for (size_t i = std::max(av.watcher_a_pos, av.watcher_b_pos) + 1; i < av.members.size(); ++i) {
        const auto chosen = try_get_chosen_goal_candidate_.try_get(av.members.at(i)->parent);
        if (!chosen)
            return i;
        if (*chosen != av.members.at(i)->idx)
            return SIZE_MAX;
    }
    return av.members.size();
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD>
std::optional<const resolution_lineage*>
dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD>::visit_avoidance(
    avoidance_id id, const resolution_lineage* rl) {
    avoidance& av = avoidances_.at(id);

    const goal_lineage* const watch_a = av.members.at(av.watcher_a_pos)->parent;
    const goal_lineage* const watch_b = av.members.at(av.watcher_b_pos)->parent;
    const bool          a_fired   = watch_a == rl->parent;
    size_t&             fired_pos = a_fired ? av.watcher_a_pos : av.watcher_b_pos;
    const size_t        other_pos = a_fired ? av.watcher_b_pos : av.watcher_a_pos;
    const goal_lineage* other_gl  = av.members.at(other_pos)->parent;

    auto& actions = frame_stack_.top().actions_;
    
    if (av.members.at(fired_pos)->idx != rl->idx) {
        watched_goals_[other_gl].erase(id);
        actions.emplace_back(avoidance_unwatch{id});
        return std::nullopt;
    }

    const size_t hit = scan(av);

    if (hit == SIZE_MAX) {
        watched_goals_[other_gl].erase(id);
        actions.emplace_back(avoidance_unwatch{id});
        return std::nullopt;
    }
    if (hit == av.members.size()) {
        watched_goals_[other_gl].erase(id);
        actions.emplace_back(avoidance_unwatch{id});
        return av.members.at(other_pos);
    }

    watched_goals_[av.members.at(hit)->parent].insert(id);
    // capture the pre-migration slot BEFORE overwriting the alias: pop must relocate
    // the member that just became unvisited (the old watched member) back out past
    // the watchers, keeping the watcher itself on its new (still-unvisited) member.
    const size_t prev_watcher_pos = fired_pos;
    fired_pos = hit;

    actions.emplace_back(avoidance_watcher_update{id, a_fired, prev_watcher_pos});
    return std::nullopt;
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD>
void dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD>::push_frame() {
    frame_stack_.push(frame{});
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD>
coroutine<const resolution_lineage*, void> dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD>::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();

    auto& parent = frame_stack_.top();

    // undo the actions in reverse order
    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);
    
    for (auto& rua : current.raised_unit_avoidance_lump) {
        if (frame_stack_.size() < rua.unit_boundary) {
            // A single-member (unit) avoidance has no second watcher and must never
            // be armed; its unit_boundary is 0 so this branch is unreachable for it.
            DEBUG_ASSERT(avoidances_.at(rua.id).watcher_b_pos != SIZE_MAX);
            link_watchers(rua.id);
            continue;
        }
        // still unit
        const auto& av = avoidances_.at(rua.id);
        const auto& rl_a = av.members.at(av.watcher_a_pos);
        // yield elimination
        co_yield rl_a;
        // bubble up the unit avoidance
        parent.raised_unit_avoidance_lump.push_back(rua);
    }
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD>
void dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD>::undo_action(const avoidance_action& action) {
    if (const auto* unwatch = std::get_if<avoidance_unwatch>(&action)) {
        link_watchers(unwatch->id);
    }
    else {
        // undo watcher_pos update (but keep watcher the same)
        // this is to make it so that the watchers that took longer to fire
        // will be the default watchers

        // follow this pattern to update the watcher_pos:
        // [a,b,c,d|(h),(f),g,e]
        // [a,b,c|(f),(h),d,g,e]
        // [a,b|(h),(f),c,d,g,e]
        // [a|(f),(h),b,c,d,g,e]
        // [(h),(f),a,b,c,d,g,e]

        const auto& wu = std::get<avoidance_watcher_update>(action);
        auto& av = avoidances_.at(wu.id);
        auto& members = av.members;
        auto& watcher_pos = wu.watcher_a_fired ? av.watcher_a_pos : av.watcher_b_pos;

        // swap the watchers
        std::swap(members.at(watcher_pos), members.at(wu.prev_watcher_pos));
        
        // update watcher_pos (but same watcher since we moved it there)
        watcher_pos = wu.prev_watcher_pos;
        
        // this whole operation SHOULD be valid since we only swap members that are both not visited yet.
        // for something to be a watcher, it must be unvisited, and given that at the current frame, we
        // know what used to be the watcher, we know that it also must be unvisited.

    }
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD>
void dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD>::link_watchers(avoidance_id id) {
    const auto& av = avoidances_.at(id);
    const auto& rl_a = av.members.at(av.watcher_a_pos);
    const auto& rl_b = av.members.at(av.watcher_b_pos);
    watched_goals_[rl_a->parent].insert(id);
    watched_goals_[rl_b->parent].insert(id);
}

#endif
