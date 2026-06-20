#ifndef RESOLUTION_MEMORY_HPP
#define RESOLUTION_MEMORY_HPP

#include <unordered_set>
#include "value_objects/lineage.hpp"
#include "value_objects/lemma.hpp"

struct resolution_memory {
    void record_resolution(const resolution_lineage*);
    void clear_recorded_resolutions();
    size_t get_resolution_count() const;
    lemma derive_resolution_lemma() const;
private:
    std::unordered_set<const resolution_lineage*> resolutions;
};

#endif
