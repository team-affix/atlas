#ifndef IMPORT_DATABASE_FROM_FILE_HPP
#define IMPORT_DATABASE_FROM_FILE_HPP

#include <cstdint>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include "../generated/CHCLexer.h"
#include "../generated/CHCParser.h"
#include "../hpp/database_visitor.hpp"

template<typename IMakeFunctor, typename IMakeVar, typename IPushDbRule>
void import_database_from_file(
    const std::string& path,
    IMakeFunctor& make_functor,
    IMakeVar& make_var,
    IPushDbRule& out,
    std::map<std::string, uint32_t>& functor_map,
    uint32_t& next_functor_id) {
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

    database_visitor dv(make_functor, make_var, out, functor_map, next_functor_id);
    dv.visitDatabase(db_ctx);
}

#endif
