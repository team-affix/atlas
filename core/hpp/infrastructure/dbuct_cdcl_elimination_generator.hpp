#ifndef DBUCT_CDCL_ELIMINATION_GENERATOR_HPP
#define DBUCT_CDCL_ELIMINATION_GENERATOR_HPP

#include <algorithm>
#include <cstddef>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/avoidance.hpp"
#include "value_objects/lemma.hpp"

// Delayed-backtracking variant of cdcl_elimination_generator. Learned avoidances
// are durable (never checkpointed) so they survive backtracking; each is
// classified by a fresh scan of the checkpointed chosen_goal_candidates instead
// of watched-literal cursors, so reapply() re-derives every forced elimination
// and realized conflict for the restored frontier at any camp depth.
template<typename ITryGetChosenGoalCandidate>
struct dbuct_cdcl_elimination_generator {
    explicit dbuct_cdcl_elimination_generator(ITryGetChosenGoalCandidate& tgcc);

    std::optional<const resolution_lineage*> learn(const lemma& l);

    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage* rl);

    std::vector<const resolution_lineage*> reapply();

    bool reapply_found_realized_conflict() const;

private:
    using avoidance_id = size_t;
    enum class scan_result { none, forced, realized };

    scan_result classify(const avoidance& av, const resolution_lineage* committing,
                         const resolution_lineage*& out_forced) const;

    std::unordered_map<avoidance_id, avoidance> avoidances_;
    size_t next_avoidance_id_ = 0;
    std::unordered_map<const goal_lineage*, std::unordered_set<avoidance_id>> goal_index_;
    bool reapply_realized_ = false;

    ITryGetChosenGoalCandidate& try_get_chosen_goal_candidate_;
};

template<typename ITryGetChosenGoalCandidate>
dbuct_cdcl_elimination_generator<ITryGetChosenGoalCandidate>::dbuct_cdcl_elimination_generator(
    ITryGetChosenGoalCandidate& tgcc)
    : try_get_chosen_goal_candidate_(tgcc) {}

template<typename ITryGetChosenGoalCandidate>
std::optional<const resolution_lineage*>
dbuct_cdcl_elimination_generator<ITryGetChosenGoalCandidate>::learn(const lemma& l) {
    const auto& resolutions = l.get_resolutions();
    if (resolutions.empty())
        return std::nullopt;

    std::vector<const resolution_lineage*> members(resolutions.begin(), resolutions.end());
    std::sort(members.begin(), members.end(),
              [](const resolution_lineage* a, const resolution_lineage* b) {
                  if (a->parent != b->parent) return a->parent < b->parent;
                  return a->idx < b->idx;
              });
    const size_t watcher_b = members.size() > 1 ? size_t{1} : size_t{0};
    const avoidance_id id = next_avoidance_id_++;
    avoidances_.emplace(id, avoidance{std::move(members), 0, watcher_b});
    for (const resolution_lineage* m : avoidances_.at(id).members)
        goal_index_[m->parent].insert(id);
    return std::nullopt;
}

template<typename ITryGetChosenGoalCandidate>
coroutine<const resolution_lineage*, void>
dbuct_cdcl_elimination_generator<ITryGetChosenGoalCandidate>::constrain(const resolution_lineage* rl) {
    const auto it = goal_index_.find(rl->parent);
    if (it == goal_index_.end())
        co_return;
    const std::vector<avoidance_id> ids(it->second.begin(), it->second.end());
    for (const avoidance_id id : ids) {
        const resolution_lineage* forced = nullptr;
        switch (classify(avoidances_.at(id), rl, forced)) {
            case scan_result::forced:   co_yield forced; break;
            case scan_result::realized: co_yield rl;     break;
            case scan_result::none:     break;
        }
    }
}

template<typename ITryGetChosenGoalCandidate>
std::vector<const resolution_lineage*>
dbuct_cdcl_elimination_generator<ITryGetChosenGoalCandidate>::reapply() {
    reapply_realized_ = false;
    std::vector<const resolution_lineage*> forced_eliminations;
    for (const auto& [id, av] : avoidances_) {
        const resolution_lineage* forced = nullptr;
        switch (classify(av, nullptr, forced)) {
            case scan_result::forced:   forced_eliminations.push_back(forced); break;
            case scan_result::realized: reapply_realized_ = true; break;
            case scan_result::none:     break;
        }
    }
    return forced_eliminations;
}

template<typename ITryGetChosenGoalCandidate>
bool dbuct_cdcl_elimination_generator<ITryGetChosenGoalCandidate>::reapply_found_realized_conflict() const {
    return reapply_realized_;
}

template<typename ITryGetChosenGoalCandidate>
typename dbuct_cdcl_elimination_generator<ITryGetChosenGoalCandidate>::scan_result
dbuct_cdcl_elimination_generator<ITryGetChosenGoalCandidate>::classify(
    const avoidance& av, const resolution_lineage* committing,
    const resolution_lineage*& out_forced) const {
    const resolution_lineage* unassigned = nullptr;
    size_t unassigned_count = 0;
    for (const resolution_lineage* m : av.members) {
        std::optional<rule_id> chosen;
        if (committing != nullptr && m->parent == committing->parent)
            chosen = committing->idx;
        else
            chosen = try_get_chosen_goal_candidate_.try_get(m->parent);

        if (!chosen.has_value()) {
            unassigned = m;
            ++unassigned_count;
        } else if (*chosen != m->idx) {
            return scan_result::none;
        }
    }
    if (unassigned_count == 0) return scan_result::realized;
    if (unassigned_count == 1) { out_forced = unassigned; return scan_result::forced; }
    return scan_result::none;
}

#endif
