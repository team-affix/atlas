#ifndef CANDIDATE_STORE_HPP
#define CANDIDATE_STORE_HPP

#include "lineage.hpp"
#include "frontier.hpp"
#include "predicate_index.hpp"
#include "defs.hpp"

struct candidate_store : frontier<std::vector<size_t>> {
    candidate_store(
        const database&,
        const goals&,
        lineage_pool&,
        const predicate_index&
    );
    size_t eliminate(const std::function<bool(const goal_lineage*, size_t)>&);
    bool unit(const goal_lineage*&, size_t&) const;
    bool conflicted() const;
#ifndef DEBUG
private:
#endif
    std::vector<std::vector<size_t>> expand(const std::vector<size_t>&, const rule&) override;

    lineage_pool& lp;
    const predicate_index& pi;
};

#endif
