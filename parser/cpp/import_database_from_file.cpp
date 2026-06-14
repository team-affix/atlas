#include <fstream>
#include <stdexcept>
#include "../hpp/import_database_from_file.hpp"
#include "../hpp/database_visitor.hpp"
#include "../generated/CHCLexer.h"
#include "../generated/CHCParser.h"
#include "infrastructure/atom_names.hpp"

void import_database_from_file(const std::string& path, i_make_functor& make_functor, i_make_var& make_var,
                               i_var_sequencer& var_seq, i_push_db_rule& out,
                               std::map<std::string, uint32_t>& atom_map, uint32_t& next_atom_id) {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("cannot open file: " + path);

    antlr4::ANTLRInputStream stream(file);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    parser.removeErrorListeners();

    auto* db_ctx = parser.database();
    if (parser.getNumberOfSyntaxErrors() > 0)
        throw std::runtime_error("parse error in: " + path);

    database_visitor dv(make_functor, make_var, var_seq, out, atom_map, next_atom_id);
    dv.visitDatabase(db_ctx);
}
