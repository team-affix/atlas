#include "value_objects/om_interval.hpp"

std::strong_ordering om_interval::operator<=>(const om_interval& other) const {
    if (open < other.open)
        return std::strong_ordering::less;
    if (other.open < open)
        return std::strong_ordering::greater;
    if (close < other.close)
        return std::strong_ordering::less;
    if (other.close < close)
        return std::strong_ordering::greater;
    return std::strong_ordering::equal;
}

bool om_interval::operator==(const om_interval& other) const {
    return (*this <=> other) == std::strong_ordering::equal;
}
