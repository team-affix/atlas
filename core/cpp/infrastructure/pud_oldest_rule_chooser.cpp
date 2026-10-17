#include "infrastructure/pud_oldest_rule_chooser.hpp"
#include "debug_assert.hpp"

pud_node_id pud_oldest_rule_chooser::choose_rule(const std::set<pud_node_id>& leaves) const {
    DEBUG_ASSERT(!leaves.empty());
    return *leaves.begin();
}
