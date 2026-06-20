#include "infrastructure/cdcl_elimination_generator.hpp"
#include <algorithm>

cdcl_elimination_generator::cdcl_elimination_generator(locator& loc)
    : set_chosen_goal_candidate_(loc.locate<i_set_chosen_goal_candidate>()),
      try_get_chosen_goal_candidate_(loc.locate<i_try_get_chosen_goal_candidate>()) {}

std::optional<const resolution_lineage*> cdcl_elimination_generator::learn(const lemma& l) {
    const auto& resolutions = l.get_resolutions();
    if (resolutions.empty())
        return std::nullopt;
    if (resolutions.size() == 1)
        return *resolutions.begin();

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
    return std::nullopt;
}

coroutine<const resolution_lineage*, void> cdcl_elimination_generator::constrain(
    const resolution_lineage* rl) {
    set_chosen_goal_candidate_.set(rl->parent, rl->idx);

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

size_t cdcl_elimination_generator::scan(const avoidance& av) const {
    for (size_t i = std::max(av.watcher_a_pos, av.watcher_b_pos) + 1; i < av.members.size(); ++i) {
        const auto chosen = try_get_chosen_goal_candidate_.try_get(av.members.at(i)->parent);
        if (!chosen)
            return i;
        if (*chosen != av.members.at(i)->idx)
            return SIZE_MAX;
    }
    return av.members.size();
}

std::optional<const resolution_lineage*> cdcl_elimination_generator::visit_avoidance(
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
        return av.members.at(other_pos);
    }

    watched_goals_[av.members.at(hit)->parent].insert(id);
    fired_pos = hit;
    return std::nullopt;
}

void cdcl_elimination_generator::cleanup() {
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
}
