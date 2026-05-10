#ifndef TRANSLATION_MAP_HPP
#define TRANSLATION_MAP_HPP

#include <cstdint>
#include <unordered_map>
#include "../domain/interfaces/i_translation_map.hpp"

struct translation_map : i_translation_map {
    void insert(uint32_t, uint32_t) override;
    bool contains(uint32_t) const override;
    uint32_t& at(uint32_t) override;
    const uint32_t& at(uint32_t) const override;
    void erase(uint32_t) override;
    void clear() override;
private:
    std::unordered_map<uint32_t, uint32_t> entries;
};

#endif
