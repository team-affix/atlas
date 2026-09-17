#ifndef PUD_DB_NODE_HPP
#define PUD_DB_NODE_HPP

#include "om_interval.hpp"
#include "pud_added_unification.hpp"

struct pud_db_node {
    om_interval interval;
    std::vector<pud_added_unification> added_unifications;
};

#endif
