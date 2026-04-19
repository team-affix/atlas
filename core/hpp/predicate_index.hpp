#ifndef PREDICATE_INDEX_HPP
#define PREDICATE_INDEX_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include "defs.hpp"

struct predicate_index {
    predicate_index(const database&);
    const std::vector<size_t>& at(const std::string& name) const;
#ifndef DEBUG
private:
#endif
    std::unordered_map<std::string, std::vector<size_t>> index;
};

#endif
