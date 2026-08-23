#ifndef LP_CDCL_ELIMINATION_GENERATOR_HPP
#define LP_CDCL_ELIMINATION_GENERATOR_HPP

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/avoidance_id.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/lp_avoidance.hpp"
#include "value_objects/lp_decision_frame.hpp"
#include "value_objects/lp_decision_frame_id.hpp"
#include "value_objects/lp_decision_frame_id_hash.hpp"

// Lazy-propagation CDCL store.
//
// Instead of one global avoidance set scanned on every resolution, each decision
// frame keeps its own vector of avoidances already reduced for that frame and
// everything below it. New avoidances enter at the root and travel down one edge
// at a time: when the decider commits to a specific child, descend() sends that
// child exactly the slice it has not seen yet, reducing or trimming each entry
// for the child's context on the way. Frames persist across sims, so a prefix
// walked before does no reduction work again.
//
// Avoidance members are the FULL decision set, never a lemma. A lemma strips
// ancestors, which hides satisfaction: if an ancestor decision later takes a
// different rule, a leaf-only avoidance names nothing that is ever resolved on
// that path and so can never be trimmed.
//
// Reductions a frame performs are permanent, but the eliminations they prove
// are not: tear-down rolls every elimination back. So a frame also keeps the
// set it has proven and re-applies it on entry. The root frame is the one
// exception -- nothing descends into it, so its own proven eliminations are not
// re-applied until the sim's first decision descends out of it. That costs
// pruning during a later sim's root phase, never correctness.
struct lp_cdcl_elimination_generator {
    lp_cdcl_elimination_generator();
    std::optional<const resolution_lineage*> learn();
    void descend(const resolution_lineage* rl);
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
    void cleanup();
private:
    using frames_t = std::unordered_map<lp_decision_frame_id, lp_decision_frame,
                                        lp_decision_frame_id_hash>;

    void receive(lp_decision_frame& child, lp_avoidance reduced);
    void arm(lp_decision_frame& f, size_t pos);
    void tombstone(lp_decision_frame& f, size_t pos);

    frames_t frames_;
    lp_decision_frame_id current_frame_id_;
    std::vector<const resolution_lineage*> pending_eliminations_;
    avoidance_id next_avoidance_id_;
};

#endif
