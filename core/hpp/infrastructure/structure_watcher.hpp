#ifndef STRUCTURE_WATCHER_HPP
#define STRUCTURE_WATCHER_HPP

#include <cstdint>
#include <vector>
#include "infrastructure/fully_persistent_array.hpp"
#include "infrastructure/incremental_set.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/om_label.hpp"

using watcher_head_id = uint32_t;

struct structure_watcher {
    structure_watcher();
    void watch(om_interval interval, watcher_head_id head, const std::vector<uint32_t>& reps);
    std::vector<watcher_head_id> note_var_bind(om_interval interval,
                                               uint32_t bound, uint32_t target);
    std::vector<watcher_head_id> note_functor_bind(om_interval interval,
                                                   uint32_t bound,
                                                   const std::vector<uint32_t>& introduced_reps);
    std::vector<watcher_head_id> heads_of(om_label open, uint32_t rep) const;
    std::vector<uint32_t> reps_of(om_label open, watcher_head_id head) const;
private:
    using head_set_t = incremental_set<watcher_head_id>;
    using rep_set_t  = incremental_set<uint32_t>;

    head_set_t query_heads(om_label open, uint32_t rep) const;
    rep_set_t  query_reps(om_label open, watcher_head_id head) const;

    fully_persistent_array<uint32_t,        head_set_t> reps_to_heads_;
    fully_persistent_array<watcher_head_id, rep_set_t>  heads_to_reps_;
};

#endif
