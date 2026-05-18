#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <vector>
#include "../interfaces/i_database.hpp"
#include "../value_objects/rule.hpp"

struct database : i_database {
    database(const std::vector<rule>&);
    const rule& at(size_t) const override;
    size_t size() const override;
private:
    std::vector<rule> db;
};

#endif
