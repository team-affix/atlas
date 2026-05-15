#ifndef CANDIDATE_INITIALIZER_HPP
#define CANDIDATE_INITIALIZER_HPP

#include <cstddef>
#include <memory>
#include <unordered_map>
#include "../domain/interfaces/i_candidate_initializer.hpp"
#include "../domain/interfaces/i_database.hpp"
#include "../domain/interfaces/i_frontier.hpp"
#include "../domain/interfaces/i_copier.hpp"

struct candidate;

struct candidate_initializer : i_candidate_initializer {
    candidate_initializer();
    void seed_expansion(const goal_lineage*) override;
    void initialize(const resolution_lineage*) override;
private:
    const i_database& db_;
    const i_frontier& frontier_;
    const i_copier& copier_;

    std::unordered_map<size_t, std::unique_ptr<candidate>>* candidates_;
};

#endif
