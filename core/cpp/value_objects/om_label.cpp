#include "value_objects/om_label.hpp"

om_label::om_label(uint64_t* rank_ptr) : rank_ptr_(rank_ptr) {}

bool om_label::operator<(const om_label& other) const {
    return *rank_ptr_ < *other.rank_ptr_;
}

const uint64_t* om_label::rank_ptr() const {
    return rank_ptr_;
}
