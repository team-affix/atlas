#include "../../hpp/infrastructure/file_logger.hpp"

file_logger::file_logger(const std::string& path)
    : file(path, std::ios::app) {}

std::ostream& file_logger::get_ostream() {
    return file;
}
