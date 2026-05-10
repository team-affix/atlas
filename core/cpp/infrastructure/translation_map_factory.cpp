#include "../../hpp/infrastructure/translation_map_factory.hpp"
#include "../../hpp/infrastructure/translation_map.hpp"

std::unique_ptr<i_translation_map> translation_map_factory::make() {
    return std::make_unique<translation_map>();
}
