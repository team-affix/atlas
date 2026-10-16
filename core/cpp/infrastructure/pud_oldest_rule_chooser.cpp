#include "infrastructure/pud_oldest_rule_chooser.hpp"
#include <algorithm>
#include "debug_assert.hpp"

pud_node_id pud_oldest_rule_chooser::choose_rule(const std::vector<pud_node_id>& leaves) const {
    DEBUG_ASSERT(!leaves.empty());
    return *std::min_element(leaves.begin(), leaves.end());
}
