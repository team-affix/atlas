#ifndef APPLICANT_FRONTIER_HPP
#define APPLICANT_FRONTIER_HPP

#include <unordered_map>
#include "../domain/interfaces/i_applicant_frontier.hpp"

struct applicant_frontier : i_applicant_frontier {
    void insert(const resolution_lineage*, applicant) override;
    bool contains(const resolution_lineage*) const override;
    applicant& at(const resolution_lineage*) override;
    const applicant& at(const resolution_lineage*) const override;
    void erase(const resolution_lineage*) override;
    void clear() override;
private:
    std::unordered_map<const resolution_lineage*, applicant> maps;
};

#endif
