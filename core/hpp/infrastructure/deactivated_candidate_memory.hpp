#ifndef DEACTIVATED_CANDIDATE_MEMORY_HPP
#define DEACTIVATED_CANDIDATE_MEMORY_HPP

#include <unordered_set>
#include "interfaces/i_deactivated_candidate_memory.hpp"

struct deactivated_candidate_memory : i_deactivated_candidate_memory {
    deactivated_candidate_memory();
    void insert(const resolution_lineage*) override;
    void clear() override;
    bool contains(const resolution_lineage*) const override;
private:
    std::unordered_set<const resolution_lineage*> deactivated_candidates;
};

#endif
