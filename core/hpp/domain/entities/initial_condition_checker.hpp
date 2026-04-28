#ifndef INITIAL_CONDITION_CHECKER_HPP
#define INITIAL_CONDITION_CHECKER_HPP

#include "../../value_objects/lineage.hpp"
#include "../candidate_store/candidate_store.hpp"
#include "../../../infrastructure/event_topic.hpp"
#include "../../events/unit_event.hpp"
#include "../../events/conflicted_event.hpp"

struct initial_condition_checker {
    initial_condition_checker();
    void operator()(const goal_lineage*);
#ifndef DEBUG
private:
#endif
    lineage_pool& lp;
    candidate_store& cs;
    event_topic<unit_event>& unit_topic;
    event_topic<conflicted_event>& conflicted_topic;
};

#endif
