#ifndef PUD_DISK_METER_HPP
#define PUD_DISK_METER_HPP

#include <cstddef>
#include <vector>

// Adds up how many bytes the registered modules' files take on disk.
template<typename IMeasureBytes>
struct pud_disk_meter {
    pud_disk_meter();
    void add(IMeasureBytes& module_files);
    size_t bytes() const;
private:
    std::vector<IMeasureBytes*> module_files_;
};

template<typename IMeasureBytes>
pud_disk_meter<IMeasureBytes>::pud_disk_meter()
    : module_files_() {}

template<typename IMeasureBytes>
void pud_disk_meter<IMeasureBytes>::add(IMeasureBytes& module_files) {
    module_files_.push_back(&module_files);
}

template<typename IMeasureBytes>
size_t pud_disk_meter<IMeasureBytes>::bytes() const {
    size_t total = 0;
    for (const IMeasureBytes* files : module_files_)
        total += files->bytes();
    return total;
}

#endif
