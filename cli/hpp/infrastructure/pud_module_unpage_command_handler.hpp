#ifndef PUD_MODULE_UNPAGE_COMMAND_HANDLER_HPP
#define PUD_MODULE_UNPAGE_COMMAND_HANDLER_HPP

#include <string>

// `atlas module unpage`: writes a paged module directory back out as a binary module file.
struct pud_module_unpage_command_handler {
    pud_module_unpage_command_handler(const std::string& input_directory, const std::string& output_path);
    void operator()();
private:
    std::string input_directory_;
    std::string output_path_;
};

#endif
