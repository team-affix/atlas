#ifndef UNIT_PROPAGATION_DETECTOR_HPP
#define UNIT_PROPAGATION_DETECTOR_HPP

#include "a01_defs.hpp"

struct unit_propagation_detector {
    unit_propagation_detector(
        const a01_candidate_store&
    );
    bool operator()(const goal_lineage*);
#ifndef DEBUG
private:
#endif
    const a01_candidate_store& cs;
};

#endif
