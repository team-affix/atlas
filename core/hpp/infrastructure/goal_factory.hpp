#ifndef GOAL_FACTORY_HPP
#define GOAL_FACTORY_HPP

#include <memory>
#include "../domain/value_objects/goal.hpp"
#include "../domain/value_objects/lineage.hpp"
#include "../domain/value_objects/expr.hpp"
#include "../domain/interfaces/i_factory.hpp"
#include "../domain/interfaces/i_lineage_pool.hpp"
#include "../domain/interfaces/i_database.hpp"

struct goal_factory : i_factory<goal, const goal_lineage*, const expr*> {
    goal_factory();
    std::unique_ptr<goal> make(const goal_lineage*, const expr*) const override;
private:
    i_factory<candidate, size_t>& candidate_factory_;
    i_lineage_pool& lp_;
    const i_database& db_;
};

#endif
