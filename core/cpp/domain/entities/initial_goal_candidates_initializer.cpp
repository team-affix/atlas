#include "../../../hpp/domain/entities/initial_goal_candidates_initializer.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"
#include "../../../hpp/domain/interfaces/i_database.hpp"

initial_goal_candidates_initializer::initial_goal_candidates_initializer() :
    gcs(resolver::resolve<i_candidates_frontier>()) {
    i_database& db = resolver::resolve<i_database>();
    for (size_t i = 0; i < db.size(); ++i)
        initial_candidates.candidates.insert(i);
}

void initial_goal_candidates_initializer::initialize(const goal_lineage* gl) {
    gcs.insert(gl, initial_candidates);
}
