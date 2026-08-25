#ifndef LP_CDCL_ELIMINATION_GENERATOR_HPP
#define LP_CDCL_ELIMINATION_GENERATOR_HPP

#include <cstddef>
#include <unordered_map>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/avoidance_id.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/lp_avoidance.hpp"
#include "value_objects/lp_avoidance_update.hpp"
#include "value_objects/lp_decision_frame.hpp"
#include "value_objects/lp_decision_frame_id.hpp"
#include "value_objects/lp_decision_frame_id_hash.hpp"
#include "value_objects/lp_timestamp.hpp"

// Lazy-propagation CDCL store.
//
// Instead of one global avoidance set scanned on every resolution, each decision
// frame keeps its own view of the avoidances that matter to it. A parent never
// works on a child's behalf: it only ever delivers updates into the child's
// mailbox, and the child merges them when the sim enters it. Frames persist
// across sims, so a prefix walked before does no reduction work again.
//
// A delivered copy is a snapshot, and snapshots rot: if a parent later reduces
// an avoidance, a child holding the older copy would sit watching a goal that
// already resolved above it and can never fire again. So every change a frame
// makes to an avoidance is re-timestamped, and each child edge is handed
// everything that moved since it last visited. Deaths travel the same channel,
// which is why no tombstone or dead-entry marker exists anywhere here.
//
// Avoidance members are the FULL decision set, never a lemma. A lemma strips
// ancestors, which hides satisfaction: if an ancestor decision later takes a
// different rule, a leaf-only avoidance names nothing that is ever resolved on
// that path and so can never be trimmed.
struct lp_cdcl_elimination_generator {
    lp_cdcl_elimination_generator();

    void learn();
    void descend(const resolution_lineage* rl);
    void enter();
    coroutine<const resolution_lineage*, void> flush();
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
    void cleanup();

private:
    using frames_t = std::unordered_map<lp_decision_frame_id, lp_decision_frame,
                                        lp_decision_frame_id_hash>;

    void merge(avoidance_id id, const lp_avoidance_update& incoming);
    void adopt(lp_avoidance reduced);
    void record_change(avoidance_id id, lp_avoidance_update update);
    void arm(avoidance_id id);
    void unarm(avoidance_id id);

    frames_t frames_;
    lp_decision_frame_id current_frame_id_;
    // Held alongside the id because hashing a frame id walks the whole decision
    // set. Re-acquired after every frames_ insert: a rehash invalidates it.
    lp_decision_frame* current_frame_;
    std::vector<const resolution_lineage*> pending_eliminations_;
    avoidance_id next_avoidance_id_;
    lp_timestamp next_timestamp_;
};

#endif
