#include <CLI/CLI.hpp>
#include "../hpp/ridge_command_handler.hpp"
#include "../hpp/horizon_command_handler.hpp"

#ifndef ATLAS_GIT_TAG
#define ATLAS_GIT_TAG "unknown"
#endif

int main(int argc, char** argv) {
    CLI::App app{"Atlas CHC Solver"};
    app.name("atlas " ATLAS_GIT_TAG);
    app.set_version_flag("-v,--version", ATLAS_GIT_TAG);
    app.require_subcommand(1);

    // --- ridge subcommand ---
    struct {
        std::string file;
        std::string goals_str;
        size_t max_resolutions      = 1000;
        double exploration_constant = 1.41;
        uint64_t seed               = 0;
    } ridge_opts;

    auto* ridge_sub = app.add_subcommand("ridge", "Run the Ridge solver");
    ridge_sub->add_option("file", ridge_opts.file, "CHC input file")->required();
    ridge_sub->add_option("-g,--goal", ridge_opts.goals_str, "Goal body string, e.g. \"(p X), (q X)\"")->required();
    ridge_sub->add_option("--max-resolutions", ridge_opts.max_resolutions, "Max resolutions");
    ridge_sub->add_option("--exploration-constant", ridge_opts.exploration_constant, "MCTS exploration constant");
    ridge_sub->add_option("--seed", ridge_opts.seed, "RNG seed");
    ridge_sub->callback([&]() {
        ridge_command_handler h(ridge_opts.file, ridge_opts.goals_str,
                                ridge_opts.max_resolutions,
                                ridge_opts.exploration_constant,
                                ridge_opts.seed);
        h();
    });

    // --- horizon subcommand ---
    struct {
        std::string file;
        std::string goals_str;
        size_t max_resolutions      = 1000;
        double exploration_constant = 1.41;
        uint64_t seed               = 0;
    } horizon_opts;

    auto* horizon_sub = app.add_subcommand("horizon", "Run the Horizon solver");
    horizon_sub->add_option("file", horizon_opts.file, "CHC input file")->required();
    horizon_sub->add_option("-g,--goal", horizon_opts.goals_str, "Goal body string, e.g. \"(p X), (q X)\"")->required();
    horizon_sub->add_option("--max-resolutions", horizon_opts.max_resolutions, "Max resolutions");
    horizon_sub->add_option("--exploration-constant", horizon_opts.exploration_constant, "MCTS exploration constant");
    horizon_sub->add_option("--seed", horizon_opts.seed, "RNG seed");
    horizon_sub->callback([&]() {
        horizon_command_handler h(horizon_opts.file, horizon_opts.goals_str,
                                  horizon_opts.max_resolutions,
                                  horizon_opts.exploration_constant,
                                  horizon_opts.seed);
        h();
    });

    CLI11_PARSE(app, argc, argv);
}
