#include "value_objects/avoidance.hpp"

avoidance::avoidance() : watcher_a_pos(0), watcher_b_pos(1) {}

avoidance::avoidance(std::vector<const resolution_lineage*> members, size_t watcher_a_pos, size_t watcher_b_pos)
    : members(std::move(members)), watcher_a_pos(watcher_a_pos), watcher_b_pos(watcher_b_pos) {}
