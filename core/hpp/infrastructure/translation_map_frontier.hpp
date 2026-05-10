#ifndef TRANSLATION_MAP_FRONTIER_HPP
#define TRANSLATION_MAP_FRONTIER_HPP

#include <unordered_map>
#include <memory>
#include "../domain/interfaces/i_translation_map_frontier.hpp"

struct translation_map_frontier : i_translation_map_frontier {
    void insert(const resolution_lineage*, std::unique_ptr<i_translation_map>) override;
    bool contains(const resolution_lineage*) const override;
    std::unique_ptr<i_translation_map>& at(const resolution_lineage*) override;
    const std::unique_ptr<i_translation_map>& at(const resolution_lineage*) const override;
    void erase(const resolution_lineage*) override;
    void clear() override;
private:
    std::unordered_map<const resolution_lineage*, std::unique_ptr<i_translation_map>> maps;
};

#endif
