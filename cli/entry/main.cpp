#include <CLI/CLI.hpp>
#include "infrastructure/basic_command_handler.hpp"
#include "infrastructure/ridge_command_handler.hpp"

#ifndef ATLAS_GIT_TAG
#define ATLAS_GIT_TAG "unknown"
#endif

int main(int argc, char** argv) {
    CLI::App app{"Atlas CHC Solver"};
    app.name("atlas");
    app.set_version_flag("-v,--version", ATLAS_GIT_TAG);
    app.require_subcommand(1);

    struct {
        std::string file;
        std::string goals_str;
        size_t max_resolutions = 1000;
        uint32_t seed          = 0;
    } basic_opts;

    auto* basic_sub = app.add_subcommand("basic", "Run the basic solver (random decisions + joint elimination)");
    basic_sub->add_option("file", basic_opts.file, "CHC input file")->required();
    basic_sub->add_option("-g,--goal", basic_opts.goals_str, "Goal body string, e.g. \"p(X), q(X)\"")->required();
    basic_sub->add_option("--max-resolutions", basic_opts.max_resolutions, "Max resolutions");
    basic_sub->add_option("--seed", basic_opts.seed, "RNG seed");
    basic_sub->callback([&]() {
        basic_command_handler h(basic_opts.file, basic_opts.goals_str,
                                basic_opts.max_resolutions, basic_opts.seed);
        h();
    });

    struct {
        std::string file;
        std::string goals_str;
        size_t max_resolutions       = 1000;
        uint32_t seed                = 0;
        double exploration_constant  = 1.414;
    } ridge_opts;

    auto* ridge_sub = app.add_subcommand("ridge", "Run the ridge solver (MCTS decisions + joint elimination)");
    ridge_sub->add_option("file", ridge_opts.file, "CHC input file")->required();
    ridge_sub->add_option("-g,--goal", ridge_opts.goals_str, "Goal body string, e.g. \"p(X), q(X)\"")->required();
    ridge_sub->add_option("--max-resolutions", ridge_opts.max_resolutions, "Max resolutions");
    ridge_sub->add_option("--seed", ridge_opts.seed, "RNG seed");
    ridge_sub->add_option("--exploration-constant", ridge_opts.exploration_constant, "MCTS exploration constant");
    ridge_sub->callback([&]() {
        ridge_command_handler h(ridge_opts.file, ridge_opts.goals_str,
                                ridge_opts.max_resolutions, ridge_opts.seed,
                                ridge_opts.exploration_constant);
        h();
    });

    CLI11_PARSE(app, argc, argv);
}
