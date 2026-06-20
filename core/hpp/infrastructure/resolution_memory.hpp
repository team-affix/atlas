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

inline void resolution_memory::record_resolution(const resolution_lineage* rl) {
    resolutions.insert(rl);
}

inline void resolution_memory::clear_recorded_resolutions() {
    resolutions.clear();
}

inline size_t resolution_memory::get_resolution_count() const {
    return resolutions.size();
}

inline lemma resolution_memory::derive_resolution_lemma() const {
    return lemma{resolutions};
}

#endif
