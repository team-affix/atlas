#ifndef DB_PRINTER_HPP
#define DB_PRINTER_HPP

#include <ostream>
#include "infrastructure/db.hpp"

template<typename IPrintRule>
struct db_printer {
    db_printer(std::ostream& os, IPrintRule& print_rule);
    void print(const db& database);
private:
    std::ostream& os_;
    IPrintRule& print_rule_;
};

template<typename IPrintRule>
db_printer<IPrintRule>::db_printer(std::ostream& os, IPrintRule& print_rule)
    : os_(os), print_rule_(print_rule) {}

template<typename IPrintRule>
void db_printer<IPrintRule>::print(const db& database) {
    auto it = database.lookup_all_rules().iterate();
    while (auto id = it.next()) {
        print_rule_.print(*database.get_rule(*id));
        os_ << "\n\n";
    }
}

#endif
