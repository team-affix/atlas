#ifndef PHOENIX_FC_RUNTIME_HPP
#define PHOENIX_FC_RUNTIME_HPP

#include <cstdint>
#include "infrastructure/phoenix_fc_manifest.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/lemma.hpp"

struct phoenix_fc_runtime {
    phoenix_fc_runtime(
        db& database,
        initial_goal_exprs& goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant);

    bool next();
    bool solved() const;
    size_t resolution_depth() const;
    size_t decision_depth() const;
    const expr* normalize(framed_expr);
    lemma derive_decision_lemma() const;
    lemma derive_resolution_lemma() const;

private:
    phoenix_fc_manifest manifest_;
};

#endif
