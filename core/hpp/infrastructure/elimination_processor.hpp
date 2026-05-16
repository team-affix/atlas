#ifndef ELIMINATION_PROCESSOR_HPP
#define ELIMINATION_PROCESSOR_HPP

#include "../interfaces/i_elimination_processor.hpp"
#include "../interfaces/i_frontier.hpp"
#include "../interfaces/i_deactivated_goal_memory.hpp"
#include "../interfaces/i_elimination_generator.hpp"
#include "../interfaces/i_elimination_backlog.hpp"
#include "../interfaces/i_candidate_deactivator.hpp"
#include "../interfaces/i_conflict_detector.hpp"

struct elimination_processor : i_elimination_processor {
    elimination_processor(
        const i_frontier&,
        const i_deactivated_goal_memory&,
        i_elimination_generator&,
        i_elimination_backlog&,
        i_candidate_deactivator&,
        i_conflict_detector&);
    bool process(const resolution_lineage*) override;
private:
    const i_frontier& frontier;
    const i_deactivated_goal_memory& dgm;
    i_elimination_generator& eg;
    i_elimination_backlog& eb;
    i_candidate_deactivator& cd;
    i_conflict_detector& conflict_detector;
};

#endif
