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

// Delayed-backtracking variant of cdcl_elimination_generator.
//
// The learned avoidances are the durable product of conflict analysis: they must
// SURVIVE backtracking (that is the whole point of re-application after camping
// backsteps). So this generator is NOT checkpointed — avoidances_ only ever
// grows via learn().
//
// The production generator uses a two-watched-literal scheme whose watcher
// cursors are incremental mutable state. Under camping those cursors would fall
// out of sync with the restored partial assignment after a backstep. This variant
// instead classifies each avoidance by a fresh scan of chosen_goal_candidates
// (which IS checkpointed), so propagation is automatically consistent with the
// current frontier at any camp depth. It exposes reapply(), which re-derives all
// forced eliminations / realized conflicts for the current frontier after a
// backtrack.
template<typename ITryGetChosenGoalCandidate>
struct dbuct_cdcl_elimination_generator {
    explicit dbuct_cdcl_elimination_generator(ITryGetChosenGoalCandidate& tgcc)
        : try_get_chosen_goal_candidate_(tgcc) {}

    // Register a conflict lemma as a persistent avoidance. Crucially this stores
    // EVERY lemma, including single-member (unit) ones: under camping a learned
    // no-good must survive arbitrary backtracking and be re-derived at every
    // resume level (reapply()), because a later backstep resurrects the candidate
    // it forbids. Storing units as avoidances (rather than routing them once and
    // forgetting them) is what keeps the "learning survives backtracking" contract
    // — and lets the cascade observe the emptied-goal conflict a unit can cause.
    //
    // Returns nullopt: application (and conflict detection) is driven uniformly by
    // reapply()/constrain(), never by a one-shot forced elimination.
    std::optional<const resolution_lineage*> learn(const lemma& l) {
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

    // In-episode propagation: react to the resolution rl that is about to be
    // committed, forcing any avoidance that becomes unit under it.
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage* rl) {
        const auto it = goal_index_.find(rl->parent);
        if (it == goal_index_.end())
            co_return;
        // Copy ids: classification does not mutate the index, but avoid iterator
        // surprises if learn() were ever interleaved.
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

    // Post-backtrack re-application: re-derive every forced elimination for the
    // current (restored) frontier and flag a realized conflict if the frontier
    // already contains a full learned no-good. Returns the forced eliminations as
    // a batch (a plain vector rather than a coroutine, so this header can be
    // instantiated across translation units without tripping GCC's coroutine
    // comdat-folding bug).
    std::vector<const resolution_lineage*> reapply() {
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

    bool reapply_found_realized_conflict() const { return reapply_realized_; }

    void cleanup() {}

private:
    using avoidance_id = size_t;
    enum class scan_result { none, forced, realized };

    // committing may be null (pure re-application) or the resolution being
    // committed (in-episode), which is treated as chosen for its goal.
    scan_result classify(const avoidance& av, const resolution_lineage* committing,
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
                return scan_result::none;  // a member diverged: avoidance can't fire
            }
        }
        if (unassigned_count == 0) return scan_result::realized;
        if (unassigned_count == 1) { out_forced = unassigned; return scan_result::forced; }
        return scan_result::none;
    }

    std::unordered_map<avoidance_id, avoidance> avoidances_;
    size_t next_avoidance_id_ = 0;
    std::unordered_map<const goal_lineage*, std::unordered_set<avoidance_id>> goal_index_;
    bool reapply_realized_ = false;

    ITryGetChosenGoalCandidate& try_get_chosen_goal_candidate_;
};

#endif
