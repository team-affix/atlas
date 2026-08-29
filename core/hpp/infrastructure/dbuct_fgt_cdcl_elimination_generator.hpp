#ifndef DBUCT_FGT_CDCL_ELIMINATION_GENERATOR_HPP
#define DBUCT_FGT_CDCL_ELIMINATION_GENERATOR_HPP

#include <algorithm>
#include <cstddef>
#include <deque>
#include <list>
#include <optional>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/avoidance.hpp"
#include "value_objects/avoidance_id.hpp"
#include "value_objects/fgt_avoidance_action.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/raised_unit_avoidance.hpp"
#include "debug_assert.hpp"

template<typename ITryGetChosenGoalCandidate,
    typename IGetPenultimateMctsFrameDepth,
    typename IDeriveDecisionLemma,
    typename IGetUltimateDecision,
    typename IGetPenultimateDecision,
    typename IGetUltimateMctsFrameDepth>
struct dbuct_fgt_cdcl_elimination_generator {
    dbuct_fgt_cdcl_elimination_generator(
        ITryGetChosenGoalCandidate&,
        IGetPenultimateMctsFrameDepth&,
        IDeriveDecisionLemma&,
        IGetUltimateDecision&,
        IGetPenultimateDecision&,
        IGetUltimateMctsFrameDepth&,
        size_t max_clauses);
    void learn();
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
    void push_frame();
    coroutine<const resolution_lineage*, void> pop_frame();
private:
    size_t scan(const avoidance& av) const;
    std::optional<const resolution_lineage*> visit_avoidance(avoidance_id, const resolution_lineage*);
    void undo_action(const fgt_avoidance_action& action);
    void link_watchers(avoidance_id id);
    void make_unevictable(avoidance_id id);
    void make_evictable(avoidance_id id);
    void evict_lru();

    std::unordered_map<avoidance_id, avoidance> avoidances_;
    size_t next_avoidance_id_;
    std::unordered_map<const goal_lineage*, std::unordered_set<avoidance_id>> watched_goals_;

    using fire_order_iter = std::list<avoidance_id>::iterator;
    std::list<avoidance_id>                           fire_order_;
    std::unordered_map<avoidance_id, fire_order_iter> fire_pos_;
    size_t                                            capacity_;

    struct frame {
        std::list<fgt_avoidance_action>  actions_;
        std::list<raised_unit_avoidance> raised_unit_avoidance_lump;
    };
    std::stack<frame> frame_stack_;

    ITryGetChosenGoalCandidate&  try_get_chosen_goal_candidate_;
    IGetPenultimateMctsFrameDepth& get_penultimate_mcts_frame_depth_;
    IDeriveDecisionLemma&        derive_decision_lemma_;
    IGetUltimateDecision&        get_ultimate_decision_;
    IGetPenultimateDecision&     get_penultimate_decision_;
    IGetUltimateMctsFrameDepth&  get_ultimate_mcts_frame_depth_;
};

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD,
         typename IGUMFD>
dbuct_fgt_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD, IGUMFD>::
dbuct_fgt_cdcl_elimination_generator(
    ITGCC& tgcc, IGUB& gub, IDL& dl, IGUD& gud, IGPD& gpd, IGUMFD& gumfd,
    size_t max_clauses)
    : next_avoidance_id_(0), capacity_(max_clauses),
      frame_stack_(std::deque<frame>{frame{}}),
      try_get_chosen_goal_candidate_(tgcc), get_penultimate_mcts_frame_depth_(gub),
      derive_decision_lemma_(dl), get_ultimate_decision_(gud),
      get_penultimate_decision_(gpd), get_ultimate_mcts_frame_depth_(gumfd) {}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD,
         typename IGUMFD>
void
dbuct_fgt_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD, IGUMFD>::learn() {
    lemma l = derive_decision_lemma_.derive_decision_lemma();

    auto resolutions = l.get_resolutions();

    if (resolutions.size() == 0)
        return;

    const avoidance_id id = next_avoidance_id_++;

    size_t unit_boundary = get_penultimate_mcts_frame_depth_.get_penultimate_mcts_frame_depth();
    frame_stack_.top().raised_unit_avoidance_lump.emplace_back(raised_unit_avoidance{id, unit_boundary});

    if (resolutions.size() == 1) {
        avoidances_.emplace(id, avoidance{{*resolutions.begin()}, 0, SIZE_MAX});
        return;
    }

    auto ultimate_decision   = get_ultimate_decision_.get_ultimate_decision();
    auto penultimate_decision = get_penultimate_decision_.get_penultimate_decision();

    resolutions.erase(ultimate_decision);
    resolutions.erase(penultimate_decision);

    std::vector<const resolution_lineage*> members(2 + resolutions.size());
    members[0] = ultimate_decision;
    members[1] = penultimate_decision;
    std::copy(resolutions.begin(), resolutions.end(), members.begin() + 2);

    avoidances_.emplace(id, avoidance{std::move(members), 0, 1});
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD,
         typename IGUMFD>
coroutine<const resolution_lineage*, void>
dbuct_fgt_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD, IGUMFD>::constrain(
    const resolution_lineage* rl) {
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

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD,
         typename IGUMFD>
size_t dbuct_fgt_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD, IGUMFD>::scan(
    const avoidance& av) const {
    for (size_t i = std::max(av.watcher_a_pos, av.watcher_b_pos) + 1; i < av.members.size(); ++i) {
        const auto chosen = try_get_chosen_goal_candidate_.try_get(av.members.at(i)->parent);
        if (!chosen)
            return i;
        if (*chosen != av.members.at(i)->idx)
            return SIZE_MAX;
    }
    return av.members.size();
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD,
         typename IGUMFD>
std::optional<const resolution_lineage*>
dbuct_fgt_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD, IGUMFD>::visit_avoidance(
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
        make_unevictable(id);
        return av.members.at(other_pos);
    }

    watched_goals_[av.members.at(hit)->parent].insert(id);
    const size_t prev_watcher_pos = fired_pos;
    fired_pos = hit;

    actions.emplace_back(avoidance_watcher_update{id, a_fired, prev_watcher_pos});
    return std::nullopt;
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD,
         typename IGUMFD>
void dbuct_fgt_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD, IGUMFD>::push_frame() {
    frame_stack_.push(frame{});
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD,
         typename IGUMFD>
coroutine<const resolution_lineage*, void>
dbuct_fgt_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD, IGUMFD>::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();

    auto& parent = frame_stack_.top();

    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);

    const size_t ultimate_mcts =
        get_ultimate_mcts_frame_depth_.get_ultimate_mcts_frame_depth();
    for (auto& rua : current.raised_unit_avoidance_lump) {
        if (ultimate_mcts < rua.unit_boundary) {
            DEBUG_ASSERT(avoidances_.at(rua.id).watcher_b_pos != SIZE_MAX);
            make_evictable(rua.id);
            link_watchers(rua.id);
            continue;
        }
        DEBUG_ASSERT(!fire_pos_.count(rua.id));
        const auto& av = avoidances_.at(rua.id);
        const auto& rl_a = av.members.at(av.watcher_a_pos);
        co_yield rl_a;
        parent.raised_unit_avoidance_lump.push_back(rua);
    }
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD,
         typename IGUMFD>
void dbuct_fgt_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD, IGUMFD>::undo_action(
    const fgt_avoidance_action& action) {
    if (const auto* unwatch = std::get_if<avoidance_unwatch>(&action)) {
        if (!avoidances_.count(unwatch->id)) return;
        link_watchers(unwatch->id);
    }
    else if (const auto* unevictable = std::get_if<avoidance_made_unevictable>(&action)) {
        make_evictable(unevictable->id);
    }
    else {
        const auto& wu = std::get<avoidance_watcher_update>(action);
        if (!avoidances_.count(wu.id)) return;
        auto& av = avoidances_.at(wu.id);
        auto& members = av.members;
        auto& watcher_pos = wu.watcher_a_fired ? av.watcher_a_pos : av.watcher_b_pos;
        std::swap(members.at(watcher_pos), members.at(wu.prev_watcher_pos));
        watcher_pos = wu.prev_watcher_pos;
    }
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD,
         typename IGUMFD>
void dbuct_fgt_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD, IGUMFD>::link_watchers(
    avoidance_id id) {
    const auto& av = avoidances_.at(id);
    const auto& rl_a = av.members.at(av.watcher_a_pos);
    const auto& rl_b = av.members.at(av.watcher_b_pos);
    watched_goals_[rl_a->parent].insert(id);
    watched_goals_[rl_b->parent].insert(id);
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD,
         typename IGUMFD>
void dbuct_fgt_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD, IGUMFD>::make_unevictable(
    avoidance_id id) {
    fire_order_.erase(fire_pos_.at(id));
    fire_pos_.erase(id);
    frame_stack_.top().actions_.emplace_back(avoidance_made_unevictable{id});
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD,
         typename IGUMFD>
void dbuct_fgt_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD, IGUMFD>::make_evictable(
    avoidance_id id) {
    if (fire_order_.size() >= capacity_)
        evict_lru();
    fire_order_.push_back(id);
    fire_pos_[id] = std::prev(fire_order_.end());
}

template<typename ITGCC, typename IGUB, typename IDL, typename IGUD, typename IGPD,
         typename IGUMFD>
void dbuct_fgt_cdcl_elimination_generator<ITGCC, IGUB, IDL, IGUD, IGPD, IGUMFD>::evict_lru() {
    const avoidance_id id = fire_order_.front();
    fire_order_.pop_front();
    fire_pos_.erase(id);
    const avoidance& av = avoidances_.at(id);
    watched_goals_[av.members.at(av.watcher_a_pos)->parent].erase(id);
    watched_goals_[av.members.at(av.watcher_b_pos)->parent].erase(id);
    avoidances_.erase(id);
}

#endif
