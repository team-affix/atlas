#ifndef CANDIDATE_STORE_HPP
#define CANDIDATE_STORE_HPP

#include "lineage.hpp"
#include "frontier.hpp"
#include "defs.hpp"

struct candidate_store : frontier<std::unordered_set<size_t>> {
    candidate_store(
        const database&,
        const goals&,
        lineage_pool&
    );
    bool conflicted() const;
#ifndef DEBUG
private:
#endif
    std::vector<std::unordered_set<size_t>> expand(const std::unordered_set<size_t>&, const rule&) override;

    const database& db;
    lineage_pool& lp;

    std::unordered_set<size_t> initial_candidates;
};

#endif
