#include "infrastructure/basic_command_handler.hpp"
#include "parser/hpp/import_database_from_file.hpp"
#include "parser/hpp/import_goals_from_string.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"
#include "interfaces/i_var_names.hpp"
#include <iostream>

basic_command_handler::basic_command_handler(
    const std::string& file,
    const std::string& goals_str,
    size_t max_resolutions,
    uint32_t seed)
    : parse_var_seq_(0) {
    parse_loc_.bind_as<i_log_to_current_trail_frame>(parse_trail_);
    parse_loc_.bind_as<i_var_names>(var_names_);
    parse_pool_.emplace(parse_loc_);
    printer_.emplace(std::cout, parse_loc_);

    import_database_from_file(file, *parse_pool_, *parse_pool_, parse_var_seq_, database_);
    var_name_to_idx_ = import_goals_from_string(goals_str, *parse_pool_, *parse_pool_, parse_var_seq_, initial_goals_);
    for (const auto& [name, idx] : var_name_to_idx_)
        var_names_.set_name(idx, name);

    const size_t initial_var_count = parse_var_seq_.peek();
    runtime_.emplace(database_, initial_goals_, initial_var_count, max_resolutions, seed);
}

void basic_command_handler::operator()() {
    solve_loop_.run(*runtime_, *printer_, *parse_pool_, var_name_to_idx_);
}
