#include "infrastructure/bt_cdcl_elimination_generator.hpp"

#include <algorithm>
#include "value_objects/resolution_lineage_ptr_less.hpp"
#include "debug_assert.hpp"

bt_cdcl_elimination_generator::bt_cdcl_elimination_generator()
    : nodes_()
    , leaves_()
    , pairs_()
    , generation_(1) {}

std::optional<const resolution_lineage*>
bt_cdcl_elimination_generator::learn(const lemma& l) {
    const auto& resolutions = l.get_resolutions();
    if (resolutions.empty())
        return std::nullopt;
    if (resolutions.size() == 1)
        return *resolutions.begin();

    std::vector<const resolution_lineage*> members(resolutions.begin(), resolutions.end());
    std::sort(members.begin(), members.end(), resolution_lineage_ptr_less{});
    bt_cdcl_factor* root = intern_members(members, 0, members.size());
    root->nand_multiplicity += 1;
    return std::nullopt;
}

coroutine<const resolution_lineage*, void>
bt_cdcl_elimination_generator::constrain(const resolution_lineage* rl) {
    std::vector<const resolution_lineage*> yields;
    const auto it = leaves_.find(rl);
    if (it != leaves_.end())
        visit_leaf(it->second, yields);
    for (const resolution_lineage* y : yields)
        co_yield y;
}

void bt_cdcl_elimination_generator::cleanup() {
    generation_ += 1;
}

bt_cdcl_factor*
bt_cdcl_elimination_generator::intern_leaf(const resolution_lineage* rl) {
    const auto it = leaves_.find(rl);
    if (it != leaves_.end())
        return it->second;
    nodes_.emplace_back(1, nullptr, nullptr, rl);
    bt_cdcl_factor* f = &nodes_.back();
    leaves_.emplace(rl, f);
    return f;
}

bt_cdcl_factor*
bt_cdcl_elimination_generator::intern_pair(bt_cdcl_factor* left, bt_cdcl_factor* right) {
    const bt_cdcl_pair_key key{left, right};

    const auto it = pairs_.find(key);
    if (it != pairs_.end())
        return it->second;

    nodes_.emplace_back(left->tuple_size + right->tuple_size, left, right, nullptr);
    bt_cdcl_factor* f = &nodes_.back();

    left->parents.push_back(f);
    right->parents.push_back(f);
    pairs_.emplace(key, f);

    return f;
}

bt_cdcl_factor*
bt_cdcl_elimination_generator::intern_members(
    const std::vector<const resolution_lineage*>& members, size_t begin, size_t end) {
    const size_t count = end - begin;
    DEBUG_ASSERT(count >= 1);
    if (count == 1)
        return intern_leaf(members.at(begin));
    const bool odd = (count % 2) == 1;
    if (odd)
        return intern_pair(intern_members(members, begin, end - 1), intern_leaf(members.at(end - 1)));
    const size_t mid = begin + count / 2;
    return intern_pair(intern_members(members, begin, mid), intern_members(members, mid, end));
}

void bt_cdcl_elimination_generator::visit_leaf(
    bt_cdcl_factor* f, std::vector<const resolution_lineage*>& yields) {
    refresh(f);
    if (f->visited >= f->tuple_size)
        return;
    f->visited += 1;
    propagate_visit(f, yields);
}

void bt_cdcl_elimination_generator::propagate_visit(
    bt_cdcl_factor* f, std::vector<const resolution_lineage*>& yields) {
    try_fire(f, yields);
    for (bt_cdcl_factor* parent : f->parents) {
        if (visited(f) != f->tuple_size) {
            try_fire(parent, yields);
            continue;
        }
        refresh(parent);
        if (parent->visited >= parent->tuple_size)
            continue;
        parent->visited += f->tuple_size;
        propagate_visit(parent, yields);
    }
}

void bt_cdcl_elimination_generator::try_fire(
    bt_cdcl_factor* f, std::vector<const resolution_lineage*>& yields) {
    if (f->nand_multiplicity == 0)
        return;
    if (f->fired_generation == generation_)
        return;
    size_t count = 0;
    const resolution_lineage* remaining = find_unvisited_leaf(f, count);
    if (count != 1)
        return;
    for (size_t i = 0; i < f->nand_multiplicity; ++i)
        yields.push_back(remaining);
    f->fired_generation = generation_;
}

const resolution_lineage*
bt_cdcl_elimination_generator::find_unvisited_leaf(const bt_cdcl_factor* f, size_t& count) const {
    if (count > 1)
        return nullptr;
    if (f->left == nullptr) {
        if (visited(f) < f->tuple_size)
            ++count;
        return (count == 1) ? f->leaf_rl : nullptr;
    }
    const resolution_lineage* lu = find_unvisited_leaf(f->left, count);
    const resolution_lineage* ru = find_unvisited_leaf(f->right, count);
    return lu ? lu : ru;
}

size_t bt_cdcl_elimination_generator::visited(const bt_cdcl_factor* f) const {
    if (f->visited_generation != generation_)
        return 0;
    return f->visited;
}

void bt_cdcl_elimination_generator::refresh(bt_cdcl_factor* f) {
    if (f->visited_generation == generation_)
        return;
    f->visited_generation = generation_;
    f->visited = 0;
}
