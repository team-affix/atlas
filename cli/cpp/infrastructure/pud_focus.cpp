#include "infrastructure/pud_focus.hpp"

#include <stdexcept>

#include "infrastructure/hex_uint64.hpp"

pud_global_node_id pud_focus::parse(const std::string& text) const {
    const size_t colon = text.rfind(':');
    const bool missing_separator = colon == std::string::npos || colon == 0 || colon + 1 == text.size();
    if (missing_separator)
        throw std::runtime_error("focus must be <module-hex>:<node>");
    const std::string module_text = text.substr(0, colon);
    const std::string node_text = text.substr(colon + 1);
    for (const char character : node_text) {
        const bool digit = character >= '0' && character <= '9';
        if (!digit)
            throw std::runtime_error("focus node is not a decimal id");
    }
    hex_uint64 hex_codec;
    const pud_module_id module = hex_codec.parse_hex(module_text);
    size_t node = 0;
    for (const char character : node_text) {
        const size_t digit = static_cast<size_t>(character - '0');
        if (node > (static_cast<size_t>(-1) - digit) / 10)
            throw std::runtime_error("focus node is not a decimal id");
        node = node * 10 + digit;
    }
    return pud_global_node_id{.module = module, .node = node};
}
