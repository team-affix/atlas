#ifndef PUD_RANDOM_UNFOLD_CHOOSER_HPP
#define PUD_RANDOM_UNFOLD_CHOOSER_HPP

#include <map>
#include <random>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_derivation_id.hpp"
#include "value_objects/pud_unfold_choice.hpp"

struct pud_random_unfold_chooser {
    pud_random_unfold_chooser(std::mt19937& rng);
    pud_unfold_choice choose_unfold(const std::map<pud_derivation_id, std::vector<pud_body_goal_id>>& frontier);
private:
    std::mt19937& rng_;
};

#endif
