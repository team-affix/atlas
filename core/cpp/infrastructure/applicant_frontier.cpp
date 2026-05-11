#include "../../hpp/infrastructure/applicant_frontier.hpp"

void applicant_frontier::insert(const resolution_lineage* rl, applicant a) {
    maps.emplace(rl, std::move(a));
}

bool applicant_frontier::contains(const resolution_lineage* rl) const {
    return maps.contains(rl);
}

applicant& applicant_frontier::at(const resolution_lineage* rl) {
    return maps.at(rl);
}

const applicant& applicant_frontier::at(const resolution_lineage* rl) const {
    return maps.at(rl);
}

void applicant_frontier::erase(const resolution_lineage* rl) {
    maps.erase(rl);
}

void applicant_frontier::clear() {
    maps.clear();
}
