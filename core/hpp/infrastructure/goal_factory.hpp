#ifndef GOAL_FACTORY_HPP
#define GOAL_FACTORY_HPP

#include <memory>
#include "../domain/interfaces/i_goal_factory.hpp"
#include "../domain/interfaces/i_candidate_factory.hpp"
#include "../domain/interfaces/i_expr_traverser_factory.hpp"
#include "../domain/interfaces/i_lineage_pool.hpp"
#include "../domain/interfaces/i_database.hpp"
#include "../domain/interfaces/i_normalizer.hpp"

struct goal_factory : i_goal_factory {
    goal_factory();
    std::unique_ptr<goal> make(const goal_lineage*, const expr*) override;
private:
    i_candidate_factory& candidate_factory_;
    i_lineage_pool& lp_;
    const i_database& db_;
    i_normalizer& normalizer_;
    i_expr_traverser_factory& traverser_factory_;
};

#endif
