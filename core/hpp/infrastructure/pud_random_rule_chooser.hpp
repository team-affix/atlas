#ifndef PUD_RANDOM_RULE_CHOOSER_HPP
#define PUD_RANDOM_RULE_CHOOSER_HPP

#include <random>
#include <set>
#include "value_objects/pud_node_id.hpp"

// Chooses one of the offered leaves uniformly at random.
struct pud_random_rule_chooser {
    pud_random_rule_chooser(std::mt19937& rng);
    pud_node_id choose_rule(const std::set<pud_node_id>& leaves);
private:
    std::mt19937& rng_;
};

#endif
