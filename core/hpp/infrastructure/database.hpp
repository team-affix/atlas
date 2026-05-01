#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <vector>
#include "../domain/interfaces/i_database.hpp"
#include "../domain/value_objects/rule.hpp"

struct database : i_database {
    database(const std::vector<rule>&);
    const rule& at(size_t) const override;
    size_t size() const override;
#ifndef DEBUG
private:
#endif
    std::vector<rule> db;
};

#endif
