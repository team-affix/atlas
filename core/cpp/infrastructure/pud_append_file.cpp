#include "infrastructure/pud_append_file.hpp"
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

pud_append_file::pud_append_file(const std::string& path)
    : descriptor_(::open(path.c_str(), O_RDWR | O_CREAT, 0644))
    , size_(0) {
    if (descriptor_ < 0)
        throw std::runtime_error("cannot open file " + path);
    struct stat status;
    ::fstat(descriptor_, &status);
    size_ = static_cast<uint64_t>(status.st_size);
}

pud_append_file::~pud_append_file() {
    ::close(descriptor_);
}

uint64_t pud_append_file::append(const std::string& bytes) {
    const uint64_t offset = size_;
    size_t written = 0;
    while (written < bytes.size()) {
        const ssize_t step = ::pwrite(
            descriptor_, bytes.data() + written, bytes.size() - written, static_cast<off_t>(offset + written));
        if (step <= 0)
            throw std::runtime_error("cannot append to file");
        written += static_cast<size_t>(step);
    }
    size_ += bytes.size();
    return offset;
}

std::string pud_append_file::read(uint64_t offset, uint64_t size) const {
    std::string bytes(size, '\0');
    size_t done = 0;
    while (done < size) {
        const ssize_t step = ::pread(descriptor_, bytes.data() + done, size - done, static_cast<off_t>(offset + done));
        if (step <= 0)
            throw std::runtime_error("cannot read file");
        done += static_cast<size_t>(step);
    }
    return bytes;
}

uint64_t pud_append_file::size() const {
    return size_;
}

void pud_append_file::sync() {
    ::fsync(descriptor_);
}
