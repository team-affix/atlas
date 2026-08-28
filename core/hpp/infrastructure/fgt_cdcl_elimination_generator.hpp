#ifndef FGT_CDCL_ELIMINATION_GENERATOR_HPP
#define FGT_CDCL_ELIMINATION_GENERATOR_HPP

#include <algorithm>
#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/avoidance.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/resolution_lineage_ptr_less.hpp"
#include "value_objects/rule.hpp"

template<typename ITryGetChosenGoalCandidate>
struct fgt_cdcl_elimination_generator {
    fgt_cdcl_elimination_generator(ITryGetChosenGoalCandidate&, size_t max_clauses);
    std::optional<const resolution_lineage*> learn(const lemma&);
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
    void cleanup();
private:
    using avoidance_id = size_t;
    using fire_order_iter = std::list<avoidance_id>::iterator;

    size_t scan(const avoidance& av) const;
    std::optional<const resolution_lineage*> visit_avoidance(avoidance_id, const resolution_lineage*);
    void evict_oldest();
    void trim_to_capacity();

    std::unordered_map<avoidance_id, avoidance> avoidances_;
    size_t next_avoidance_id_;
    std::unordered_map<const goal_lineage*, std::unordered_set<avoidance_id>> watched_goals_;
    std::unordered_set<avoidance_id> visited_avoidances_;

    // front = least recently fired (evict here); back = most recently fired
    std::list<avoidance_id> fire_order_;
    std::unordered_map<avoidance_id, fire_order_iter> fire_pos_;

    size_t capacity_;
    ITryGetChosenGoalCandidate& try_get_chosen_goal_candidate_;
};

template<typename ITGCC>
fgt_cdcl_elimination_generator<ITGCC>::fgt_cdcl_elimination_generator(
    ITGCC& tgcc, size_t max_clauses)
    : next_avoidance_id_(0)
    , capacity_(max_clauses)
    , try_get_chosen_goal_candidate_(tgcc) {}

template<typename ITGCC>
std::optional<const resolution_lineage*>
fgt_cdcl_elimination_generator<ITGCC>::learn(const lemma& l) {
    const auto& resolutions = l.get_resolutions();
    if (resolutions.empty())
        return std::nullopt;
    if (resolutions.size() == 1)
        return *resolutions.begin();

    std::vector<const resolution_lineage*> members(resolutions.begin(), resolutions.end());
    std::sort(members.begin(), members.end(), resolution_lineage_ptr_less{});
    const avoidance_id id = next_avoidance_id_++;
    avoidances_.emplace(id, avoidance{std::move(members), 0, 1});
    watched_goals_[avoidances_.at(id).members.at(0)->parent].insert(id);
    watched_goals_[avoidances_.at(id).members.at(1)->parent].insert(id);

    // new avoidances go to the back so they are not immediately evicted
    fire_order_.push_back(id);
    fire_pos_[id] = std::prev(fire_order_.end());

    return std::nullopt;
}

template<typename ITGCC>
coroutine<const resolution_lineage*, void>
fgt_cdcl_elimination_generator<ITGCC>::constrain(const resolution_lineage* rl) {
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

template<typename ITGCC>
size_t fgt_cdcl_elimination_generator<ITGCC>::scan(const avoidance& av) const {
    for (size_t i = std::max(av.watcher_a_pos, av.watcher_b_pos) + 1; i < av.members.size(); ++i) {
        const auto chosen = try_get_chosen_goal_candidate_.try_get(av.members.at(i)->parent);
        if (!chosen)
            return i;
        if (*chosen != av.members.at(i)->idx)
            return SIZE_MAX;
    }
    return av.members.size();
}

template<typename ITGCC>
std::optional<const resolution_lineage*>
fgt_cdcl_elimination_generator<ITGCC>::visit_avoidance(
    avoidance_id id, const resolution_lineage* rl) {
    avoidance& av = avoidances_.at(id);
    visited_avoidances_.insert(id);

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
        // this avoidance just fired: move it to the back (most recently fired)
        fire_order_.splice(fire_order_.end(), fire_order_, fire_pos_[id]);
        return av.members.at(other_pos);
    }

    watched_goals_[av.members.at(hit)->parent].insert(id);
    fired_pos = hit;
    return std::nullopt;
}

template<typename ITGCC>
void fgt_cdcl_elimination_generator<ITGCC>::evict_oldest() {
    const avoidance_id id = fire_order_.front();
    const avoidance& av = avoidances_.at(id);

    watched_goals_[av.members.at(0)->parent].erase(id);
    watched_goals_[av.members.at(1)->parent].erase(id);

    avoidances_.erase(id);
    fire_pos_.erase(id);
    fire_order_.pop_front();
}

template<typename ITGCC>
void fgt_cdcl_elimination_generator<ITGCC>::trim_to_capacity() {
    while (avoidances_.size() > capacity_)
        evict_oldest();
}

template<typename ITGCC>
void fgt_cdcl_elimination_generator<ITGCC>::cleanup() {
    for (const avoidance_id id : visited_avoidances_) {
        avoidance& av = avoidances_.at(id);
        std::swap(av.members.at(0), av.members.at(av.watcher_a_pos));
        std::swap(av.members.at(1), av.members.at(av.watcher_b_pos));
        av.watcher_a_pos = 0;
        av.watcher_b_pos = 1;
        watched_goals_[av.members.at(0)->parent].insert(id);
        watched_goals_[av.members.at(1)->parent].insert(id);
    }
    visited_avoidances_.clear();

    trim_to_capacity();
}

#endif
