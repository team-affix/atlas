#ifndef OM_LABEL_HPP
#define OM_LABEL_HPP

#include <cstdint>

struct om_label {
    om_label(uint64_t* rank_ptr);
    bool operator<(const om_label& other) const;
    const uint64_t* rank_ptr() const;
private:
    uint64_t* rank_ptr_;
};

#endif
