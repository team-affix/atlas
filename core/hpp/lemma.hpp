#ifndef LEMMA_HPP
#define LEMMA_HPP

#include "defs.hpp"

struct lemma {
    lemma(const resolutions&);
    const resolutions& get() const;
#ifndef DEBUG
private:
#endif
    void remove_ancestors(const resolution_lineage*, std::set<const resolution_lineage*>&);

    resolutions value;
};

#endif
