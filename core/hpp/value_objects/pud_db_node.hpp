#ifndef PUD_DB_NODE_HPP
#define PUD_DB_NODE_HPP

#include "pud_added_unification.hpp"

struct pud_db_node {
    std::vector<pud_added_unification> added_unifications;
};

#endif
