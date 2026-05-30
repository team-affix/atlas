#include <fstream>
#include <stdexcept>
#include "../hpp/import_database_from_file.hpp"
#include "../hpp/database_visitor.hpp"
#include "../generated/CHCLexer.h"
#include "../generated/CHCParser.h"

database import_database_from_file(const std::string& path, expr_pool& pool, sequencer& seq) {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("cannot open file: " + path);

    antlr4::ANTLRInputStream stream(file);
    CHCLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CHCParser parser(&tokens);
    parser.removeErrorListeners();

    auto* db = parser.database();
    if (parser.getNumberOfSyntaxErrors() > 0)
        throw std::runtime_error("parse error in: " + path);

    database_visitor dv(pool, seq);
    return std::any_cast<database>(dv.visitDatabase(db));
}
