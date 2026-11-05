#ifndef PUD_NAME_LOG_HPP
#define PUD_NAME_LOG_HPP

#include <cstdint>
#include <string>
#include <vector>

// A module's functor names kept as a log of length-prefixed entries in an append-only file: each name is appended as
// it is learned, and all are read back on open, in the order they were appended.
template<typename IAppendBytes, typename IReadBytes>
struct pud_name_log {
    pud_name_log(IAppendBytes& append_bytes, IReadBytes& read_bytes);
    std::vector<std::string> read_all() const;
    void append(const std::string& name);
private:
    IAppendBytes& append_bytes_;
    IReadBytes& read_bytes_;
};

template<typename IAppendBytes, typename IReadBytes>
pud_name_log<IAppendBytes, IReadBytes>::pud_name_log(IAppendBytes& append_bytes, IReadBytes& read_bytes)
    : append_bytes_(append_bytes)
    , read_bytes_(read_bytes) {}

template<typename IAppendBytes, typename IReadBytes>
std::vector<std::string> pud_name_log<IAppendBytes, IReadBytes>::read_all() const {
    const std::string bytes = read_bytes_.read(0, read_bytes_.size());
    std::vector<std::string> names;
    size_t at = 0;
    while (at + sizeof(uint64_t) <= bytes.size()) {
        uint64_t length = 0;
        for (size_t byte = 0; byte < sizeof(uint64_t); ++byte)
            length |= static_cast<uint64_t>(static_cast<uint8_t>(bytes.at(at + byte))) << (8 * byte);
        at += sizeof(uint64_t);
        names.push_back(bytes.substr(at, length));
        at += length;
    }
    return names;
}

template<typename IAppendBytes, typename IReadBytes>
void pud_name_log<IAppendBytes, IReadBytes>::append(const std::string& name) {
    std::string entry;
    const uint64_t length = name.size();
    for (size_t byte = 0; byte < sizeof(uint64_t); ++byte)
        entry.push_back(static_cast<char>((length >> (8 * byte)) & 0xff));
    entry += name;
    append_bytes_.append(entry);
}

#endif
