#ifndef LEMMA_HPP
#define LEMMA_HPP

#include <unordered_set>
#include "lineage.hpp"

struct lemma {
    lemma(const std::unordered_set<const resolution_lineage*>&);
    const std::unordered_set<const resolution_lineage*>& get_resolutions() const;
private:
    void remove_ancestors(const resolution_lineage*, std::unordered_set<const resolution_lineage*>&);
    std::unordered_set<const resolution_lineage*> rs;
};

#endif
