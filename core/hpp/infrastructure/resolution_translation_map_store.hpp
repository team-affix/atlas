#ifndef RESOLUTION_TRANSLATION_MAP_STORE_HPP
#define RESOLUTION_TRANSLATION_MAP_STORE_HPP

#include <unordered_map>
#include "../domain/interfaces/i_resolution_translation_map_store.hpp"

struct resolution_translation_map_store : i_resolution_translation_map_store {
    void insert(const resolution_lineage*, translation_map) override;
    translation_map& at(const resolution_lineage*) override;
    const translation_map& at(const resolution_lineage*) const override;
    void erase(const resolution_lineage*) override;
    void clear() override;
private:
    std::unordered_map<const resolution_lineage*, translation_map> maps;
};

#endif
