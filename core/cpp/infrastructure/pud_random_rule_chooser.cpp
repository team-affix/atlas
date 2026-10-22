#include "infrastructure/pud_random_rule_chooser.hpp"
#include <iterator>
#include "debug_assert.hpp"

pud_random_rule_chooser::pud_random_rule_chooser(std::mt19937& rng)
    : rng_(rng) {}

pud_node_id pud_random_rule_chooser::choose_rule(const std::set<pud_node_id>& leaves) {
    DEBUG_ASSERT(!leaves.empty());
    std::uniform_int_distribution<size_t> pick(0, leaves.size() - 1);
    return *std::next(leaves.begin(), pick(rng_));
}
