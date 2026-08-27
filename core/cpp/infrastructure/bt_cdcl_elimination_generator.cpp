#include "infrastructure/bt_cdcl_elimination_generator.hpp"

#include <algorithm>
#include <functional>
#include "debug_assert.hpp"

namespace {

bool bt_cdcl_member_less(const resolution_lineage* a, const resolution_lineage* b) {
    if (a->parent != b->parent)
        return a->parent < b->parent;
    return a->idx < b->idx;
}

}

bt_cdcl_elimination_generator::factor::factor(
    size_t tuple_size,
    factor* left,
    factor* right,
    const resolution_lineage* leaf_rl)
    : tuple_size_(tuple_size)
    , visited_(0)
    , visited_generation_(0)
    , nand_multiplicity_(0)
    , fired_generation_(0)
    , left_(left)
    , right_(right)
    , leaf_rl_(leaf_rl)
    , parents_() {}

bool bt_cdcl_elimination_generator::pair_key::operator==(const pair_key& o) const {
    return left == o.left && right == o.right;
}

size_t bt_cdcl_elimination_generator::pair_key_hash::operator()(const pair_key& k) const {
    const size_t h0 = std::hash<factor*>{}(k.left);
    const size_t h1 = std::hash<factor*>{}(k.right);
    return h0 ^ (h1 + 0x9e3779b97f4a7c15ULL + (h0 << 6) + (h0 >> 2));
}

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
    std::sort(members.begin(), members.end(), bt_cdcl_member_less);
    factor* root = build(members, 0, members.size());
    root->nand_multiplicity_ += 1;
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

bt_cdcl_elimination_generator::factor*
bt_cdcl_elimination_generator::intern_leaf(const resolution_lineage* rl) {
    const auto it = leaves_.find(rl);
    if (it != leaves_.end())
        return it->second;
    nodes_.emplace_back(1, nullptr, nullptr, rl);
    factor* f = &nodes_.back();
    leaves_.emplace(rl, f);
    return f;
}

bt_cdcl_elimination_generator::factor*
bt_cdcl_elimination_generator::intern_pair(factor* left, factor* right) {
    const pair_key key{left, right};
    const auto it = pairs_.find(key);
    if (it != pairs_.end())
        return it->second;
    nodes_.emplace_back(left->tuple_size_ + right->tuple_size_, left, right, nullptr);
    factor* f = &nodes_.back();
    left->parents_.push_back(f);
    right->parents_.push_back(f);
    pairs_.emplace(key, f);
    return f;
}

bt_cdcl_elimination_generator::factor*
bt_cdcl_elimination_generator::build(
    const std::vector<const resolution_lineage*>& members, size_t begin, size_t end) {
    const size_t n = end - begin;
    DEBUG_ASSERT(n >= 1);
    if (n == 1)
        return intern_leaf(members.at(begin));
    if ((n % 2) == 1)
        return intern_pair(build(members, begin, end - 1), intern_leaf(members.at(end - 1)));
    const size_t mid = begin + n / 2;
    return intern_pair(build(members, begin, mid), build(members, mid, end));
}

size_t bt_cdcl_elimination_generator::live_visited(const factor* f) const {
    if (f->visited_generation_ != generation_)
        return 0;
    return f->visited_;
}

void bt_cdcl_elimination_generator::ensure_generation(factor* f) {
    if (f->visited_generation_ == generation_)
        return;
    f->visited_generation_ = generation_;
    f->visited_ = 0;
}

void bt_cdcl_elimination_generator::visit_leaf(
    factor* f, std::vector<const resolution_lineage*>& yields) {
    ensure_generation(f);
    if (f->visited_ >= f->tuple_size_)
        return;
    f->visited_ += 1;
    after_increment(f, yields);
}

void bt_cdcl_elimination_generator::after_increment(
    factor* f, std::vector<const resolution_lineage*>& yields) {
    check_nand(f, yields);
    if (live_visited(f) != f->tuple_size_) {
        for (factor* parent : f->parents_)
            check_nand(parent, yields);
        return;
    }
    for (factor* parent : f->parents_) {
        ensure_generation(parent);
        if (parent->visited_ >= parent->tuple_size_)
            continue;
        parent->visited_ += f->tuple_size_;
        after_increment(parent, yields);
    }
}

void bt_cdcl_elimination_generator::check_nand(
    factor* f, std::vector<const resolution_lineage*>& yields) {
    if (f->nand_multiplicity_ == 0)
        return;
    if (f->fired_generation_ == generation_)
        return;
    if (unvisited_count(f) != 1)
        return;
    const resolution_lineage* remaining = unvisited_leaf(f);
    DEBUG_ASSERT(remaining != nullptr);
    for (size_t i = 0; i < f->nand_multiplicity_; ++i)
        yields.push_back(remaining);
    f->fired_generation_ = generation_;
}

size_t bt_cdcl_elimination_generator::unvisited_count(const factor* f) const {
    if (f->left_ == nullptr)
        return live_visited(f) >= f->tuple_size_ ? 0 : 1;
    return unvisited_count(f->left_) + unvisited_count(f->right_);
}

const resolution_lineage*
bt_cdcl_elimination_generator::unvisited_leaf(const factor* f) const {
    if (f->left_ == nullptr) {
        if (live_visited(f) >= f->tuple_size_)
            return nullptr;
        return f->leaf_rl_;
    }
    if (const resolution_lineage* u = unvisited_leaf(f->left_))
        return u;
    return unvisited_leaf(f->right_);
}
