#ifndef PUD_MAPPED_RECORD_FILE_HPP
#define PUD_MAPPED_RECORD_FILE_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include "value_objects/pud_node_record.hpp"

// A file of node records mapped into memory, opened or created at a path. Records are read in place and written
// through the mapping, so the operating system's page cache holds whichever are in use and writes them back. The file
// grows by doubling as records are appended; a reference to a record lasts until the next append.
struct pud_mapped_record_file {
    pud_mapped_record_file(const std::string& path);
    pud_mapped_record_file(const pud_mapped_record_file&) = delete;
    pud_mapped_record_file& operator=(const pud_mapped_record_file&) = delete;
    ~pud_mapped_record_file();
    size_t count() const;
    const pud_node_record& get(size_t index) const;
    void set(size_t index, const pud_node_record& record);
    void append(const pud_node_record& record);
    void sync();
    size_t bytes() const;
private:
    struct file_header {
        char magic[8];
        uint64_t count;
    };

    void map(size_t capacity);
    file_header& header() const;
    pud_node_record* records() const;

    int descriptor_;
    void* mapping_;
    size_t capacity_;
};

#endif
