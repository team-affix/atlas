#ifndef IMPORT_DATABASE_FROM_FILE_HPP
#define IMPORT_DATABASE_FROM_FILE_HPP

#include <cstdint>
#include <map>
#include <string>
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"
#include "interfaces/i_var_sequencer.hpp"
#include "interfaces/i_push_db_rule.hpp"

void import_database_from_file(
    const std::string& path,
    i_make_functor&,
    i_make_var&,
    i_var_sequencer&,
    i_push_db_rule&,
    std::map<std::string, uint32_t>& atom_map,
    uint32_t& next_atom_id);

#endif
