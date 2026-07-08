#ifndef DBUCT_CDCL_ELIMINATION_GENERATOR_HPP
#define DBUCT_CDCL_ELIMINATION_GENERATOR_HPP

#include <algorithm>
#include <cstddef>
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
#include "value_objects/avoidance_visitation.hpp"
#include "debug_assert.hpp"

// NOTES: learn() can be void since learn() is always expected to be called immediately before pop(),
// since learn() is only ever invoked at a terminal state where there is nothing left to do.
template<typename ITryGetChosenGoalCandidate, typename IGetUnitBoundary, typename IDeriveDecisionLemma, typename IGetUltimateDecision>
struct dbuct_cdcl_elimination_generator {
    dbuct_cdcl_elimination_generator(ITryGetChosenGoalCandidate&, IGetUnitBoundary&, IDeriveDecisionLemma&, IGetUltimateDecision&);
    void learn();
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
    void push_frame();
    coroutine<const resolution_lineage*, void> pop_frame();
private:
    size_t scan(const avoidance& av) const;
    std::optional<const resolution_lineage*> visit_avoidance(avoidance_id, const resolution_lineage*);
    void raise_elimination();

    std::unordered_map<avoidance_id, avoidance> avoidances_;
    size_t next_avoidance_id_ = 0;
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
        std::list<avoidance_visitation> avoidance_visitations;
    };
    std::vector<frame> frame_stack_;

    // if ever we are strictly above the unit boundary, this elimination no longer applies and
    // should be erased. else, apply the elimination (yield it)
    struct raised_elimination {
        const resolution_lineage* lineage;
        size_t unit_boundary;
    };
    std::list<raised_elimination> raised_eliminations_;
    
    ITryGetChosenGoalCandidate& try_get_chosen_goal_candidate_;
    IGetUnitBoundary& get_unit_boundary_;
    IDeriveDecisionLemma& derive_decision_lemma_;
    IGetUltimateDecision& get_ultimate_decision_;
};

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD>
dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD>::dbuct_cdcl_elimination_generator(ITGCC& tgcc, IGUB& gub, IDL& dl, IGUD& gud)
    : try_get_chosen_goal_candidate_(tgcc), get_unit_boundary_(gub), derive_decision_lemma_(dl), get_ultimate_decision_(gud), frame_stack_(1) {}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD>
void
dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD>::learn() {
    // get the decision lemma from the current position
    lemma l = derive_decision_lemma_.derive_decision_lemma();
    
    const auto& resolutions = l.get_resolutions();
    
    if (resolutions.size() == 0)
        return;

    // raise the ultimate decision as an elimination
    raise_elimination();

    // if size==1, then no avoidance needs to be created, we just float this elimination
    // to the top.
    if (resolutions.size() == 1)
        return;

    std::vector<const resolution_lineage*> members(resolutions.begin(), resolutions.end());
    std::sort(members.begin(), members.end(), [](const resolution_lineage* a, const resolution_lineage* b) {
        if (a->parent != b->parent)
            return a->parent < b->parent;
        return a->idx < b->idx;
    });
    
    const avoidance_id id = next_avoidance_id_++;
    avoidances_.emplace(id, avoidance{std::move(members), 0, 1});

    watched_goals_[avoidances_.at(id).members.at(0)->parent].insert(id);
    watched_goals_[avoidances_.at(id).members.at(1)->parent].insert(id);

    // add this learnt avoidance to the frame stack so when we backtrack above the unit boundary,
    // we 
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD>
coroutine<const resolution_lineage*, void>
dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD>::constrain(const resolution_lineage* rl) {
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

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD>
size_t dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD>::scan(const avoidance& av) const {
    for (size_t i = std::max(av.watcher_a_pos, av.watcher_b_pos) + 1; i < av.members.size(); ++i) {
        const auto chosen = try_get_chosen_goal_candidate_.try_get(av.members.at(i)->parent);
        if (!chosen)
            return i;
        if (*chosen != av.members.at(i)->idx)
            return SIZE_MAX;
    }
    return av.members.size();
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD>
std::optional<const resolution_lineage*>
dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD>::visit_avoidance(
    avoidance_id id, const resolution_lineage* rl) {
    avoidance& av = avoidances_.at(id);

    const goal_lineage* const watch_a = av.members.at(av.watcher_a_pos)->parent;
    const goal_lineage* const watch_b = av.members.at(av.watcher_b_pos)->parent;
    const bool          a_fired   = watch_a == rl->parent;
    size_t&             fired_pos = a_fired ? av.watcher_a_pos : av.watcher_b_pos;
    const size_t        other_pos = a_fired ? av.watcher_b_pos : av.watcher_a_pos;
    const goal_lineage* other_gl  = av.members.at(other_pos)->parent;

    if (av.members.at(fired_pos)->idx != rl->idx) {
        watched_goals_[other_gl].erase(id);
        return std::nullopt;
    }

    const size_t hit = scan(av);
    if (hit == SIZE_MAX) {
        watched_goals_[other_gl].erase(id);
        return std::nullopt;
    }
    if (hit == av.members.size()) {
        watched_goals_[other_gl].erase(id);
        return av.members.at(other_pos);
    }

    watched_goals_[av.members.at(hit)->parent].insert(id);
    fired_pos = hit;
    return std::nullopt;
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD>
void dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD>::raise_elimination() {
    auto unit_boundary = get_unit_boundary_.get_unit_boundary();
    auto ultimate_decision = get_ultimate_decision_.get_ultimate_decision();
    raised_eliminations_.emplace_back(ultimate_decision, unit_boundary);
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD>
void dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD>::push_frame() {
    
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD>
void dbuct_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD>::pop_frame() {
    
}

#endif
