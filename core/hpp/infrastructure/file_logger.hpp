#ifndef FILE_LOGGER_HPP
#define FILE_LOGGER_HPP

#include <fstream>
#include "../domain/interfaces/i_logger.hpp"

struct file_logger : i_logger {
    file_logger(const std::string&);
    std::ostream& get_ostream() override;
private:
    std::ofstream file;
};

#endif
