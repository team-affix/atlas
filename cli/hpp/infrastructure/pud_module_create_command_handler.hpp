#ifndef PUD_MODULE_CREATE_COMMAND_HANDLER_HPP
#define PUD_MODULE_CREATE_COMMAND_HANDLER_HPP

#include <string>
#include <vector>
#include "value_objects/pud_module_id.hpp"

struct pud_module_create_command_handler {
    pud_module_create_command_handler(
        const std::string& database_path,
        std::vector<pud_module_id> dependencies,
        const std::string& output_path);

    void operator()();

private:
    std::string database_path_;
    std::string output_path_;
    std::vector<pud_module_id> dependencies_;
};

#endif
