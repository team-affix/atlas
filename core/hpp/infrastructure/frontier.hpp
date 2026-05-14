#ifndef FRONTIER_HPP
#define FRONTIER_HPP

#include <memory>
#include <unordered_map>
#include "../domain/interfaces/i_frontier.hpp"

struct frontier : i_frontier {
    void insert(const goal_lineage*, std::unique_ptr<goal>) override;
    bool contains(const goal_lineage*) const override;
    std::unique_ptr<goal>& at(const goal_lineage*) override;
    const std::unique_ptr<goal>& at(const goal_lineage*) const override;
    void erase(const goal_lineage*) override;
    void clear() override;
    size_t size() const override;
    void accept(i_visitor<const std::pair<const goal_lineage* const, std::unique_ptr<goal>>&>&) const override;
private:
    std::unordered_map<const goal_lineage*, std::unique_ptr<goal>> goals_;
};

#endif
