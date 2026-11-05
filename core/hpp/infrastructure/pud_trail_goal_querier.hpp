#ifndef PUD_TRAIL_GOAL_QUERIER_HPP
#define PUD_TRAIL_GOAL_QUERIER_HPP

#include <cstdint>
#include <optional>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_candidate_trail.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_trails.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Finds a goal's candidates now, from the trails an earlier query of it left when there are any. A goal without
// trails is queried fresh. A trail whose goal kept its structure and whose witness leaves are all still live is the
// same candidate, its path read from parent links without a replay; any other trail is resumed.
template<
    typename IQueryGoal,
    typename IResumeTrail,
    typename IFindAncestorPath,
    typename IIsNodeLeaf,
    typename IIsNodeRefuted>
struct pud_trail_goal_querier {
    pud_trail_goal_querier(
        IQueryGoal& query_goal,
        IResumeTrail& resume_trail,
        IFindAncestorPath& find_ancestor_path,
        IIsNodeLeaf& is_node_leaf,
        IIsNodeRefuted& is_node_refuted);
    std::vector<pud_eager_candidate> query(
        pud_module_id module,
        const pud_ubs_state& caller_bindings,
        const expr* goal,
        uint32_t frame_offset,
        const pud_goal_trails* trails) const;
private:
    IQueryGoal& query_goal_;
    IResumeTrail& resume_trail_;
    IFindAncestorPath& find_ancestor_path_;
    IIsNodeLeaf& is_node_leaf_;
    IIsNodeRefuted& is_node_refuted_;
};

#define PUD_TRAIL_GOAL_QUERIER_TEMPLATE                                                                  \
    template<typename IQueryGoal, typename IResumeTrail, typename IFindAncestorPath, typename IIsNodeLeaf, \
             typename IIsNodeRefuted>
#define PUD_TRAIL_GOAL_QUERIER                                                                           \
    pud_trail_goal_querier<IQueryGoal, IResumeTrail, IFindAncestorPath, IIsNodeLeaf, IIsNodeRefuted>

PUD_TRAIL_GOAL_QUERIER_TEMPLATE
PUD_TRAIL_GOAL_QUERIER::pud_trail_goal_querier(
    IQueryGoal& query_goal,
    IResumeTrail& resume_trail,
    IFindAncestorPath& find_ancestor_path,
    IIsNodeLeaf& is_node_leaf,
    IIsNodeRefuted& is_node_refuted)
    : query_goal_(query_goal)
    , resume_trail_(resume_trail)
    , find_ancestor_path_(find_ancestor_path)
    , is_node_leaf_(is_node_leaf)
    , is_node_refuted_(is_node_refuted) {}

PUD_TRAIL_GOAL_QUERIER_TEMPLATE
std::vector<pud_eager_candidate> PUD_TRAIL_GOAL_QUERIER::query(
    pud_module_id module,
    const pud_ubs_state& caller_bindings,
    const expr* goal,
    uint32_t frame_offset,
    const pud_goal_trails* trails) const {
    if (trails == nullptr)
        return query_goal_.query(module, caller_bindings, goal, frame_offset);
    std::vector<pud_eager_candidate> candidates;
    for (const pud_candidate_trail& trail : trails->candidates) {
        bool witnesses_live = true;
        for (const pud_global_node_id leaf : trail.witness_leaves)
            witnesses_live = witnesses_live && is_node_leaf_.is_leaf(leaf) && !is_node_refuted_.is_refuted(leaf);
        if (!trails->forked && witnesses_live) {
            candidates.push_back(pud_eager_candidate{
                .path = find_ancestor_path_.find_path_from_root(trail.candidate),
                .witness_leaves = trail.witness_leaves});
            continue;
        }
        std::optional<pud_eager_candidate> resumed =
            resume_trail_.resume(module, caller_bindings, goal, frame_offset, trail);
        if (!resumed.has_value())
            continue;
        candidates.push_back(std::move(resumed.value()));
    }
    return candidates;
}

#undef PUD_TRAIL_GOAL_QUERIER
#undef PUD_TRAIL_GOAL_QUERIER_TEMPLATE

#endif
