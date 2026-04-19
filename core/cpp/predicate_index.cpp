#include "../hpp/predicate_index.hpp"

predicate_index::predicate_index(const database& db) {
    for (size_t i = 0; i < db.size(); ++i)
        index[db.at(i).head->name].push_back(i);
}

const std::vector<size_t>& predicate_index::at(const std::string& name) const {
    return index.at(name);
}
