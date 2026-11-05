#ifndef PUD_MODULE_PAGE_COMMAND_HANDLER_HPP
#define PUD_MODULE_PAGE_COMMAND_HANDLER_HPP

#include <string>

// `atlas module page`: writes a binary module file out as a paged module directory.
struct pud_module_page_command_handler {
    pud_module_page_command_handler(const std::string& input_path, const std::string& output_directory);
    void operator()();
private:
    std::string input_path_;
    std::string output_directory_;
};

#endif
