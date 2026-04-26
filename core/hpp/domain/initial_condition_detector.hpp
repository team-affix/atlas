#ifndef INITIAL_CONDITION_DETECTOR_HPP
#define INITIAL_CONDITION_DETECTOR_HPP

#include "lineage.hpp"
#include "candidate_store.hpp"
#include "topic.hpp"

struct initial_condition_detector {
    initial_condition_detector();
    bool operator()();
#ifndef DEBUG
private:
#endif
    lineage_pool& lp;
    candidate_store& cs;
    topic<const resolution_lineage*>& unit_topic;

    topic<const goal_lineage*>::subscription goal_inserted_subscription;
};

#endif
