#ifndef UNIFIER_FRONTIER_HPP
#define UNIFIER_FRONTIER_HPP

#include <unordered_map>
#include <memory>
#include "../domain/interfaces/i_unifier_frontier.hpp"

struct unifier_frontier : i_unifier_frontier {
    void insert(const resolution_lineage*, std::unique_ptr<i_unifier>) override;
    bool contains(const resolution_lineage*) const override;
    std::unique_ptr<i_unifier>& at(const resolution_lineage*) override;
    const std::unique_ptr<i_unifier>& at(const resolution_lineage*) const override;
    void erase(const resolution_lineage*) override;
    void clear() override;
private:
    std::unordered_map<const resolution_lineage*, std::unique_ptr<i_unifier>> maps;
};

#endif
