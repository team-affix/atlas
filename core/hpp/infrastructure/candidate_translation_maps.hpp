#ifndef CANDIDATE_TRANSLATION_MAPS_HPP
#define CANDIDATE_TRANSLATION_MAPS_HPP

#include <unordered_map>
#include "interfaces/i_get_candidate_translation_map.hpp"
#include "interfaces/i_set_candidate_translation_map.hpp"
#include "interfaces/i_unset_candidate_translation_map.hpp"
#include "interfaces/i_clear_candidate_translation_maps.hpp"

struct candidate_translation_maps
    : i_get_candidate_translation_map
    , i_set_candidate_translation_map
    , i_unset_candidate_translation_map
    , i_clear_candidate_translation_maps {
    translation_map& get(const resolution_lineage*) override;
    void set(const resolution_lineage*, translation_map) override;
    void unset(const resolution_lineage*) override;
    void clear_candidate_translation_maps() override;
private:
    std::unordered_map<const resolution_lineage*, translation_map> maps_;
};

#endif
