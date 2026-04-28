#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <vector>
#include "../value_objects/rule.hpp"

struct database {
    database(const std::vector<rule>&);
    const std::vector<rule>& get() const;
#ifndef DEBUG
private:
#endif
    std::vector<rule> db;
};

#endif
