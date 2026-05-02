#include "../../../hpp/domain/data_structures/database.hpp"

database::database(const std::vector<rule>& db) :
    db(db) {
}

const std::vector<rule>& database::get() const {
    return db;
}
