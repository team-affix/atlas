#ifndef PUD_FOCUS_PARSER_HPP
#define PUD_FOCUS_PARSER_HPP

#include <string>
#include "value_objects/pud_global_node_id.hpp"

struct pud_focus_parser {
    pud_global_node_id parse(const std::string& text) const;
};

#endif
