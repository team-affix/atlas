#ifndef AVOIDANCE_TRIMMER_HPP
#define AVOIDANCE_TRIMMER_HPP

#include "defs.hpp"

struct avoidance_trimmer {
    avoidance_trimmer(
        avoidance_store&,
        const avoidance_map&
    );
    void operator()(const resolution_lineage*);
#ifndef DEBUG
private:
#endif
    avoidance_store& as;
    const avoidance_map& am;
};

#endif
