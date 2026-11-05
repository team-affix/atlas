#ifndef PUD_APPEND_FILE_HPP
#define PUD_APPEND_FILE_HPP

#include <cstdint>
#include <string>

// A file that only grows, opened or created at a path: bytes are appended at its end and read back at the offset
// their append returned. The operating system's page cache keeps the parts in use.
struct pud_append_file {
    pud_append_file(const std::string& path);
    pud_append_file(const pud_append_file&) = delete;
    pud_append_file& operator=(const pud_append_file&) = delete;
    ~pud_append_file();
    uint64_t append(const std::string& bytes);
    std::string read(uint64_t offset, uint64_t size) const;
    uint64_t size() const;
    void sync();
private:
    int descriptor_;
    uint64_t size_;
};

#endif
