#ifndef CANDIDATE_STORE_HPP
#define CANDIDATE_STORE_HPP

#include "lineage.hpp"
#include "frontier.hpp"
#include "defs.hpp"

struct candidate_store : frontier<std::vector<size_t>> {
    virtual ~candidate_store() = default;
    candidate_store(
        const database&,
        const goals&,
        lineage_pool&,
        trail&
    );
    size_t eliminate(const std::function<bool(const goal_lineage*, size_t)>&);
    bool unit(const goal_lineage*&, size_t&) const;
    bool conflicted() const;
#ifndef DEBUG
private:
#endif
    std::optional<std::vector<std::vector<size_t>>> expand(const std::vector<size_t>&, const rule&) override;

    std::vector<size_t> initial_candidates;
};

#endif
