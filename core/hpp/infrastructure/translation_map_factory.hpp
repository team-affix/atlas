#ifndef TRANSLATION_MAP_FACTORY_HPP
#define TRANSLATION_MAP_FACTORY_HPP

#include <memory>
#include "../domain/interfaces/i_factory.hpp"
#include "../domain/interfaces/i_translation_map.hpp"

struct translation_map_factory : i_factory<i_translation_map> {
    std::unique_ptr<i_translation_map> make() override;
};

#endif
