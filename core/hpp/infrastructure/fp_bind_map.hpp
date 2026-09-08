#ifndef FP_BIND_MAP_HPP
#define FP_BIND_MAP_HPP

#include <map>
#include <optional>
#include <unordered_map>
#include "value_objects/om_label.hpp"
#include "value_objects/framed_expr.hpp"

// fp_bind_map: a fully persistent bind map for tree-shaped binding environments.
//
// Each call to record(open, close, var_id, value) writes two events into the
// per-variable timeline:
//   open  → value          (binding becomes active when entering this interval)
//   close → prior_value    (binding is shadowed when the interval is exited)
//
// query(open_label, var_id) performs a predecessor search on var_id's timeline
// at position open_label and returns whatever value was in effect there, or
// nullopt if the variable was unbound at that position.
//
// om_label's operator< reads the current rank through a pointer, remaining
// valid through order_maintenance relabeling.

struct fp_bind_map {
    fp_bind_map();
    void record(om_label open, om_label close, uint32_t var_id, framed_expr value);
    std::optional<framed_expr> query(om_label open_label, uint32_t var_id) const;
private:
    using timeline_t = std::map<om_label, std::optional<framed_expr>>;
    std::unordered_map<uint32_t, timeline_t> timelines_;
};

#endif
