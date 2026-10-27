#ifndef PUD_MODULE_INSPECT_COMMAND_HANDLER_HPP
#define PUD_MODULE_INSPECT_COMMAND_HANDLER_HPP

#include <string>

struct pud_module_inspect_command_handler {
    pud_module_inspect_command_handler(const std::string& input_path);

    void operator()();

private:
    std::string input_path_;
};

#endif
