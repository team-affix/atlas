#ifndef DBUCT_HORIZON_FC_RUNTIME_HPP
#define DBUCT_HORIZON_FC_RUNTIME_HPP

#include <cstddef>
#include <cstdint>
#include "infrastructure/dbuct_horizon_fc_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/lemma.hpp"

// dbuct-horizon-fc runtime — camping DBUCT + horizon (CGW) reward + fewer-candidate rollout.
struct dbuct_horizon_fc_runtime {
    static constexpr size_t k_default_grant_increment_interval = 4;

    dbuct_horizon_fc_runtime(
        db& database,
        initial_goal_exprs& goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant,
        size_t grant_increment_interval = k_default_grant_increment_interval);

    bool next();
    bool solved() const;
    const expr* normalize(framed_expr fe);
    size_t resolution_depth() const;
    size_t decision_depth() const;
    double cgw() const;
    lemma derive_decision_lemma() const;
    lemma derive_resolution_lemma() const;

private:
    dbuct_horizon_fc_manifest manifest_;
};

#endif
