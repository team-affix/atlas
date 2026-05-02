#include "../../../../hpp/domain/entities/goals/goals.hpp"

goals::goals(const std::vector<goal>& gls) :
    gls(gls) {
}

const std::vector<goal>& goals::get() const {
    return gls;
}
