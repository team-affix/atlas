#include <CLI/CLI.hpp>
#include "../../core/hpp/trail.hpp"
#include "../../core/hpp/expr.hpp"
#include "../../core/hpp/sequencer.hpp"
#include "../../parser/hpp/import_database_from_file.hpp"

int main(int argc, char** argv) {
    CLI::App app{"CHC Solver"};
    std::string filename;
    app.add_option("file", filename, "CHC input file")->required();
    CLI11_PARSE(app, argc, argv);

    trail t;
    expr_pool pool(t);
    sequencer seq(t);
    database db = import_database_from_file(filename, pool, seq);

    // TODO: hand off to solver
    (void)db;
    return 0;
}
