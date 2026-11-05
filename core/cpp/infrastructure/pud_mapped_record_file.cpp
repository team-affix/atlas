#include "infrastructure/pud_mapped_record_file.hpp"
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "debug_assert.hpp"

namespace {

constexpr char k_record_file_magic[8] = {'P', 'U', 'D', 'R', 'E', 'C', '0', '1'};
constexpr size_t k_first_capacity = 1024;
constexpr size_t k_header_bytes = 64;

size_t file_bytes_for(size_t capacity) {
    return k_header_bytes + capacity * sizeof(pud_node_record);
}

}

pud_mapped_record_file::pud_mapped_record_file(const std::string& path)
    : descriptor_(::open(path.c_str(), O_RDWR | O_CREAT, 0644))
    , mapping_(nullptr)
    , capacity_(0) {
    if (descriptor_ < 0)
        throw std::runtime_error("cannot open record file " + path);
    struct stat status;
    ::fstat(descriptor_, &status);
    const bool fresh = status.st_size == 0;
    const size_t mapped_capacity =
        fresh ? 0 : (static_cast<size_t>(status.st_size) - k_header_bytes) / sizeof(pud_node_record);
    map(mapped_capacity < k_first_capacity ? k_first_capacity : mapped_capacity);
    if (fresh) {
        std::memcpy(header().magic, k_record_file_magic, sizeof(k_record_file_magic));
        header().count = 0;
    }
    if (std::memcmp(header().magic, k_record_file_magic, sizeof(k_record_file_magic)) != 0)
        throw std::runtime_error("not a record file: " + path);
}

pud_mapped_record_file::~pud_mapped_record_file() {
    if (mapping_ != nullptr)
        ::munmap(mapping_, file_bytes_for(capacity_));
    ::close(descriptor_);
}

size_t pud_mapped_record_file::count() const {
    return header().count;
}

const pud_node_record& pud_mapped_record_file::get(size_t index) const {
    DEBUG_ASSERT(index < count());
    return records()[index];
}

void pud_mapped_record_file::set(size_t index, const pud_node_record& record) {
    DEBUG_ASSERT(index < count());
    records()[index] = record;
}

void pud_mapped_record_file::append(const pud_node_record& record) {
    const size_t index = count();
    if (index == capacity_)
        map(capacity_ * 2);
    records()[index] = record;
    header().count = index + 1;
}

void pud_mapped_record_file::sync() {
    ::msync(mapping_, file_bytes_for(capacity_), MS_SYNC);
}

size_t pud_mapped_record_file::bytes() const {
    return file_bytes_for(capacity_);
}

// Sizes the file for a capacity and maps all of it, replacing any earlier mapping.
void pud_mapped_record_file::map(size_t capacity) {
    if (::ftruncate(descriptor_, static_cast<off_t>(file_bytes_for(capacity))) != 0)
        throw std::runtime_error("cannot grow record file");
    void* mapped = mapping_ == nullptr
        ? ::mmap(nullptr, file_bytes_for(capacity), PROT_READ | PROT_WRITE, MAP_SHARED, descriptor_, 0)
        : ::mremap(mapping_, file_bytes_for(capacity_), file_bytes_for(capacity), MREMAP_MAYMOVE);
    if (mapped == MAP_FAILED)
        throw std::runtime_error("cannot map record file");
    mapping_ = mapped;
    capacity_ = capacity;
}

pud_mapped_record_file::file_header& pud_mapped_record_file::header() const {
    return *static_cast<file_header*>(mapping_);
}

pud_node_record* pud_mapped_record_file::records() const {
    return reinterpret_cast<pud_node_record*>(static_cast<char*>(mapping_) + k_header_bytes);
}
