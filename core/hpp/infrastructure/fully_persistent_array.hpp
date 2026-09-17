#ifndef FULLY_PERSISTENT_ARRAY_HPP
#define FULLY_PERSISTENT_ARRAY_HPP

#include <map>
#include <optional>
#include <unordered_map>
#include "value_objects/om_interval.hpp"
#include "value_objects/framed_expr.hpp"

// fully_persistent_array: a fully persistent array for tree-shaped version
// histories, indexed by a uint32_t key and Om-tour position.
//
// Each call to record(interval, var_id, value) writes two events into the
// per-variable timeline:
//   interval.open  → value          (binding becomes active when entering)
//   interval.close → prior_value    (binding is shadowed when exiting)
//
// query(open_label, var_id) performs a predecessor search on var_id's timeline
// at position open_label and returns whatever value was in effect there, or
// nullopt if the variable was unbound at that position.
//
// om_label's operator< reads the current rank through a pointer, remaining
// valid through order_maintenance relabeling.

struct fully_persistent_array {
    fully_persistent_array();
    void record(om_interval interval, uint32_t var_id, framed_expr value);
    std::optional<framed_expr> query(om_label open_label, uint32_t var_id) const;
private:
    using timeline_t = std::map<om_label, std::optional<framed_expr>>;
    std::unordered_map<uint32_t, timeline_t> timelines_;
};

#endif
