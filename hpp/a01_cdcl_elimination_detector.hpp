#ifndef A01_CDCL_ELIMINATION_DETECTOR_HPP
#define A01_CDCL_ELIMINATION_DETECTOR_HPP

#include "a01_defs.hpp"
#include "lineage.hpp"

struct a01_cdcl_elimination_detector {
    a01_cdcl_elimination_detector(
        const a01_avoidance_store&,
        lineage_pool&
    );
    bool operator()(const goal_lineage*, size_t);
#ifndef DEBUG
private:
#endif
    const a01_avoidance_store& as;
    lineage_pool& lp;
};

#endif
