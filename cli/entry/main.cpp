#include <CLI/CLI.hpp>
#include "infrastructure/basic_command_handler.hpp"
#include "infrastructure/genius_command_handler.hpp"
#include "infrastructure/horizon_command_handler.hpp"
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
        size_t max_resolutions       = 1000;
        uint32_t seed                = 0;
        size_t sim_progress_interval = 1000;
    } basic_opts;

    auto* basic_sub = app.add_subcommand("basic", "Run the basic solver (random decisions + joint elimination)");
    basic_sub->add_option("file", basic_opts.file, "CHC input file")->required();
    basic_sub->add_option("-g,--goal", basic_opts.goals_str, "Goal body string, e.g. \"p(X), q(X)\"")->required();
    basic_sub->add_option("--max-resolutions", basic_opts.max_resolutions, "Max resolutions");
    basic_sub->add_option("--seed", basic_opts.seed, "RNG seed");
    basic_sub->add_option("--sim-progress-interval", basic_opts.sim_progress_interval,
                          "Print sim progress every N sims (0 disables)");
    basic_sub->callback([&]() {
        basic_command_handler h(basic_opts.file, basic_opts.goals_str,
                                basic_opts.max_resolutions, basic_opts.seed,
                                basic_opts.sim_progress_interval);
        h();
    });

    struct {
        std::string file;
        std::string goals_str;
        size_t max_resolutions       = 1000;
        uint32_t seed                = 0;
        double exploration_constant  = 1.414;
        size_t sim_progress_interval = 1000;
    } ridge_opts;

    auto* ridge_sub = app.add_subcommand("ridge", "Run the ridge solver (MCTS decisions + joint elimination)");
    ridge_sub->add_option("file", ridge_opts.file, "CHC input file")->required();
    ridge_sub->add_option("-g,--goal", ridge_opts.goals_str, "Goal body string, e.g. \"p(X), q(X)\"")->required();
    ridge_sub->add_option("--max-resolutions", ridge_opts.max_resolutions, "Max resolutions");
    ridge_sub->add_option("--seed", ridge_opts.seed, "RNG seed");
    ridge_sub->add_option("--exploration-constant", ridge_opts.exploration_constant, "MCTS exploration constant");
    ridge_sub->add_option("--sim-progress-interval", ridge_opts.sim_progress_interval,
                          "Print sim progress every N sims (0 disables)");
    ridge_sub->callback([&]() {
        ridge_command_handler h(ridge_opts.file, ridge_opts.goals_str,
                                ridge_opts.max_resolutions, ridge_opts.seed,
                                ridge_opts.exploration_constant,
                                ridge_opts.sim_progress_interval);
        h();
    });

    struct {
        std::string file;
        std::string goals_str;
        size_t max_resolutions       = 1000;
        uint32_t seed                = 0;
        double exploration_constant  = 1.414;
        size_t sim_progress_interval = 1000;
    } horizon_opts;

    auto* horizon_sub = app.add_subcommand("horizon", "Run the horizon solver (MCTS + goal weights + joint elimination)");
    horizon_sub->add_option("file", horizon_opts.file, "CHC input file")->required();
    horizon_sub->add_option("-g,--goal", horizon_opts.goals_str, "Goal body string, e.g. \"p(X), q(X)\"")->required();
    horizon_sub->add_option("--max-resolutions", horizon_opts.max_resolutions, "Max resolutions");
    horizon_sub->add_option("--seed", horizon_opts.seed, "RNG seed");
    horizon_sub->add_option("--exploration-constant", horizon_opts.exploration_constant, "MCTS exploration constant");
    horizon_sub->add_option("--sim-progress-interval", horizon_opts.sim_progress_interval,
                          "Print sim progress every N sims (0 disables)");
    horizon_sub->callback([&]() {
        horizon_command_handler h(horizon_opts.file, horizon_opts.goals_str,
                                  horizon_opts.max_resolutions, horizon_opts.seed,
                                  horizon_opts.exploration_constant,
                                  horizon_opts.sim_progress_interval);
        h();
    });

    struct {
        std::string file;
        std::string goals_str;
        size_t max_resolutions       = 1000;
        uint32_t seed                = 0;
        double exploration_constant  = 1.414;
        size_t sim_progress_interval = 1000;
    } genius_opts;

    auto* genius_sub = app.add_subcommand("genius", "Run the genius solver (MCTS + per-node reward: cgw for rule nodes, ridge for goal nodes)");
    genius_sub->add_option("file", genius_opts.file, "CHC input file")->required();
    genius_sub->add_option("-g,--goal", genius_opts.goals_str, "Goal body string, e.g. \"p(X), q(X)\"")->required();
    genius_sub->add_option("--max-resolutions", genius_opts.max_resolutions, "Max resolutions");
    genius_sub->add_option("--seed", genius_opts.seed, "RNG seed");
    genius_sub->add_option("--exploration-constant", genius_opts.exploration_constant, "MCTS exploration constant");
    genius_sub->add_option("--sim-progress-interval", genius_opts.sim_progress_interval,
                          "Print sim progress every N sims (0 disables)");
    genius_sub->callback([&]() {
        genius_command_handler h(genius_opts.file, genius_opts.goals_str,
                                 genius_opts.max_resolutions, genius_opts.seed,
                                 genius_opts.exploration_constant,
                                 genius_opts.sim_progress_interval);
        h();
    });

    CLI11_PARSE(app, argc, argv);
}
