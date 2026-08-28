#include "infrastructure/fgt_bt_cdcl_elimination_generator.hpp"

#include <algorithm>
#include "value_objects/resolution_lineage_ptr_less.hpp"
#include "debug_assert.hpp"

fgt_bt_cdcl_elimination_generator::fgt_bt_cdcl_elimination_generator(size_t max_clauses)
    : nodes_()
    , node_iters_()
    , leaves_()
    , pairs_()
    , generation_(1)
    , fire_order_()
    , fire_pos_()
    , capacity_(max_clauses) {}

std::optional<const resolution_lineage*>
fgt_bt_cdcl_elimination_generator::learn(const lemma& l) {
    const auto& resolutions = l.get_resolutions();

    if (resolutions.empty())
        return std::nullopt;

    if (resolutions.size() == 1)
        return *resolutions.begin();

    std::vector<const resolution_lineage*> members(resolutions.begin(), resolutions.end());

    std::sort(members.begin(), members.end(), resolution_lineage_ptr_less{});

    bt_cdcl_factor* root = intern_members(members, 0, members.size());

    const auto it = fire_pos_.find(root);
    if (it != fire_pos_.end()) {
        fire_order_.splice(fire_order_.end(), fire_order_, it->second);
    } else {
        fire_order_.push_back(root);
        fire_pos_[root] = std::prev(fire_order_.end());
    }

    root->is_avoidance = true;

    return std::nullopt;
}

coroutine<const resolution_lineage*, void>
fgt_bt_cdcl_elimination_generator::constrain(const resolution_lineage* rl) {
    const auto it = leaves_.find(rl);

    if (it == leaves_.end())
        co_return;

    auto coro = visit_leaf(it->second);

    while (auto lineage = coro.next())
        co_yield *lineage;
}

void fgt_bt_cdcl_elimination_generator::cleanup() {
    ++generation_;
    trim_to_capacity();
}

bt_cdcl_factor*
fgt_bt_cdcl_elimination_generator::intern_leaf(const resolution_lineage* rl) {
    const auto it = leaves_.find(rl);

    if (it != leaves_.end())
        return it->second;

    nodes_.emplace_back(1, nullptr, nullptr, rl);

    bt_cdcl_factor* f = &nodes_.back();

    node_iters_[f] = std::prev(nodes_.end());
    leaves_.emplace(rl, f);

    return f;
}

bt_cdcl_factor*
fgt_bt_cdcl_elimination_generator::intern_pair(bt_cdcl_factor* left, bt_cdcl_factor* right) {
    const bt_cdcl_pair_key key{left, right};

    const auto it = pairs_.find(key);

    if (it != pairs_.end())
        return it->second;

    nodes_.emplace_back(left->tuple_size + right->tuple_size, left, right, nullptr);

    bt_cdcl_factor* f = &nodes_.back();

    node_iters_[f] = std::prev(nodes_.end());
    left->parents.push_back(f);
    right->parents.push_back(f);
    pairs_.emplace(key, f);

    return f;
}

bt_cdcl_factor*
fgt_bt_cdcl_elimination_generator::intern_members(
    const std::vector<const resolution_lineage*>& members, size_t begin, size_t end) {
    const size_t member_count = end - begin;

    DEBUG_ASSERT(member_count >= 1);

    if (member_count == 1)
        return intern_leaf(members.at(begin));

    const bool odd_count = (member_count % 2) == 1;

    if (odd_count)
        return intern_pair(intern_members(members, begin, end - 1), intern_leaf(members.at(end - 1)));

    const size_t mid = begin + member_count / 2;

    return intern_pair(intern_members(members, begin, mid), intern_members(members, mid, end));
}

coroutine<const resolution_lineage*, void>
fgt_bt_cdcl_elimination_generator::visit_leaf(bt_cdcl_factor* f) {
    refresh(f);

    if (f->visited >= f->tuple_size)
        co_return;

    ++f->visited;

    auto coro = propagate_visit(f);

    while (auto lineage = coro.next())
        co_yield *lineage;
}

coroutine<const resolution_lineage*, void>
fgt_bt_cdcl_elimination_generator::propagate_visit(bt_cdcl_factor* f) {
    auto self_fire = try_fire(f);

    while (auto lineage = self_fire.next())
        co_yield *lineage;

    for (bt_cdcl_factor* parent : f->parents) {
        if (visited(f) != f->tuple_size) {
            auto parent_fire = try_fire(parent);

            while (auto lineage = parent_fire.next())
                co_yield *lineage;

            continue;
        }

        refresh(parent);

        if (parent->visited >= parent->tuple_size)
            continue;

        parent->visited += f->tuple_size;

        auto prop = propagate_visit(parent);

        while (auto lineage = prop.next())
            co_yield *lineage;
    }
}

coroutine<const resolution_lineage*, void>
fgt_bt_cdcl_elimination_generator::try_fire(bt_cdcl_factor* f) {
    if (!f->is_avoidance)
        co_return;

    if (f->fired_generation == generation_)
        co_return;

    size_t count = 0;

    const resolution_lineage* remaining = find_unvisited_leaf(f, count);

    if (count != 1)
        co_return;

    f->fired_generation = generation_;

    const auto it = fire_pos_.find(f);
    if (it != fire_pos_.end())
        fire_order_.splice(fire_order_.end(), fire_order_, it->second);

    co_yield remaining;
}

const resolution_lineage*
fgt_bt_cdcl_elimination_generator::find_unvisited_leaf(const bt_cdcl_factor* f, size_t& count) const {
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

size_t fgt_bt_cdcl_elimination_generator::visited(const bt_cdcl_factor* f) const {
    if (f->visited_generation != generation_)
        return 0;

    return f->visited;
}

void fgt_bt_cdcl_elimination_generator::refresh(bt_cdcl_factor* f) {
    if (f->visited_generation == generation_)
        return;

    f->visited_generation = generation_;
    f->visited = 0;
}

void fgt_bt_cdcl_elimination_generator::trim(bt_cdcl_factor* node) {
    if (!node->parents.empty())
        return;

    if (node->is_avoidance)
        return;

    bt_cdcl_factor* left  = node->left;
    bt_cdcl_factor* right = node->right;

    if (left == nullptr) {
        leaves_.erase(node->leaf_rl);
    } else {
        auto& lp = left->parents;
        lp.erase(std::find(lp.begin(), lp.end(), node));

        auto& rp = right->parents;
        rp.erase(std::find(rp.begin(), rp.end(), node));

        pairs_.erase(bt_cdcl_pair_key{left, right});
    }

    const auto nit = node_iters_.find(node);
    nodes_.erase(nit->second);
    node_iters_.erase(nit);

    if (left != nullptr) {
        trim(left);
        trim(right);
    }
}

void fgt_bt_cdcl_elimination_generator::evict_oldest() {
    bt_cdcl_factor* root = fire_order_.front();

    root->is_avoidance = false;
    fire_pos_.erase(root);
    fire_order_.pop_front();

    trim(root);
}

void fgt_bt_cdcl_elimination_generator::trim_to_capacity() {
    while (fire_order_.size() > capacity_)
        evict_oldest();
}
