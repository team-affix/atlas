#ifndef DBUCT_BT_CDCL_ELIMINATION_GENERATOR_HPP
#define DBUCT_BT_CDCL_ELIMINATION_GENERATOR_HPP

#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <list>
#include <optional>
#include <stack>
#include <unordered_map>
#include <variant>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

// NOTES: learn() can be void since learn() is always expected to be called immediately before pop(),
// since learn() is only ever invoked at a terminal state where there is nothing left to do.
template<typename ITryGetChosenGoalCandidate,
    typename IGetPenultimateMctsFrameDepth,
    typename IDeriveDecisionLemma,
    typename IGetUltimateDecision,
    typename IGetUltimateMctsFrameDepth>
struct dbuct_bt_cdcl_elimination_generator {
    dbuct_bt_cdcl_elimination_generator(
        ITryGetChosenGoalCandidate&,
        IGetPenultimateMctsFrameDepth&,
        IDeriveDecisionLemma&,
        IGetUltimateDecision&,
        IGetUltimateMctsFrameDepth&);
    void learn();
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*);
    void push_frame();
    coroutine<const resolution_lineage*, void> pop_frame();
private:
    struct factor {
        factor(size_t tuple_size,
               factor* left,
               factor* right,
               const resolution_lineage* leaf_rl);
        friend struct dbuct_bt_cdcl_elimination_generator;
    private:
        size_t tuple_size_;
        size_t visited_;
        size_t nand_multiplicity_;
        bool nand_fired_;
        bool armed_;
        factor* left_;
        factor* right_;
        const resolution_lineage* leaf_rl_;
        std::vector<factor*> parents_;
    };

    struct pair_key {
        factor* left;
        factor* right;
        bool operator==(const pair_key&) const;
    };

    struct pair_key_hash {
        size_t operator()(const pair_key&) const;
    };

    struct visit_delta {
        factor* node;
        size_t amount;
    };

    struct nand_fired {
        factor* node;
    };

    using action = std::variant<visit_delta, nand_fired>;

    struct raised_unit {
        factor* nand;
        size_t unit_boundary;
        const resolution_lineage* ultimate;
    };

    struct frame {
        frame();
        std::list<action> actions_;
        std::list<raised_unit> raised_units_;
    };

    static bool member_less(const resolution_lineage*, const resolution_lineage*);
    factor* intern_leaf(const resolution_lineage*);
    factor* intern_pair(factor*, factor*);
    factor* build(const std::vector<const resolution_lineage*>&, size_t, size_t);
    void arm_catchup(factor*, std::vector<const resolution_lineage*>&);
    size_t unvisited_count(const factor*) const;
    void log_visit(factor*, size_t);
    void visit_leaf(factor*, std::vector<const resolution_lineage*>&);
    void after_increment(factor*, std::vector<const resolution_lineage*>&);
    void check_nand(factor*, std::vector<const resolution_lineage*>&);
    const resolution_lineage* unvisited_leaf(const factor*) const;
    void undo_action(const action&);

    std::deque<factor> nodes_;
    std::unordered_map<const resolution_lineage*, factor*> leaves_;
    std::unordered_map<pair_key, factor*, pair_key_hash> pairs_;
    std::stack<frame> frame_stack_;

    ITryGetChosenGoalCandidate& try_get_chosen_goal_candidate_;
    IGetPenultimateMctsFrameDepth& get_penultimate_mcts_frame_depth_;
    IDeriveDecisionLemma& derive_decision_lemma_;
    IGetUltimateDecision& get_ultimate_decision_;
    IGetUltimateMctsFrameDepth& get_ultimate_mcts_frame_depth_;
};

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::factor::factor(
    size_t tuple_size,
    factor* left,
    factor* right,
    const resolution_lineage* leaf_rl)
    : tuple_size_(tuple_size)
    , visited_(0)
    , nand_multiplicity_(0)
    , nand_fired_(false)
    , armed_(false)
    , left_(left)
    , right_(right)
    , leaf_rl_(leaf_rl)
    , parents_() {}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
bool dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::pair_key::operator==(
    const pair_key& o) const {
    return left == o.left && right == o.right;
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
size_t dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::pair_key_hash::operator()(
    const pair_key& k) const {
    const size_t h0 = std::hash<factor*>{}(k.left);
    const size_t h1 = std::hash<factor*>{}(k.right);
    return h0 ^ (h1 + 0x9e3779b97f4a7c15ULL + (h0 << 6) + (h0 >> 2));
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::frame::frame()
    : actions_()
    , raised_units_() {}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::
dbuct_bt_cdcl_elimination_generator(
    ITGCC& tgcc, IGPMFD& gpmfd, IDL& dl, IGUD& gud, IGUMFD& gumfd)
    : nodes_()
    , leaves_()
    , pairs_()
    , frame_stack_(std::deque<frame>{frame{}})
    , try_get_chosen_goal_candidate_(tgcc)
    , get_penultimate_mcts_frame_depth_(gpmfd)
    , derive_decision_lemma_(dl)
    , get_ultimate_decision_(gud)
    , get_ultimate_mcts_frame_depth_(gumfd) {}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
bool dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::member_less(
    const resolution_lineage* a, const resolution_lineage* b) {
    if (a->parent != b->parent)
        return a->parent < b->parent;
    return a->idx < b->idx;
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
void
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::learn() {
    lemma l = derive_decision_lemma_.derive_decision_lemma();
    const auto& resolutions = l.get_resolutions();
    if (resolutions.empty())
        return;

    if (resolutions.size() == 1) {
        frame_stack_.top().raised_units_.push_back(raised_unit{
            nullptr, 1, *resolutions.begin()});
        return;
    }

    const size_t unit_boundary =
        get_penultimate_mcts_frame_depth_.get_penultimate_mcts_frame_depth();
    const resolution_lineage* ultimate = get_ultimate_decision_.get_ultimate_decision();

    std::vector<const resolution_lineage*> members(resolutions.begin(), resolutions.end());
    std::sort(members.begin(), members.end(), member_less);
    factor* root = build(members, 0, members.size());
    root->nand_multiplicity_ += 1;
    frame_stack_.top().raised_units_.push_back(raised_unit{root, unit_boundary, ultimate});
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
coroutine<const resolution_lineage*, void>
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::constrain(
    const resolution_lineage* rl) {
    std::vector<const resolution_lineage*> yields;
    const auto it = leaves_.find(rl);
    if (it != leaves_.end())
        visit_leaf(it->second, yields);
    for (const resolution_lineage* y : yields)
        co_yield y;
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
void
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::push_frame() {
    frame_stack_.push(frame{});
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
coroutine<const resolution_lineage*, void>
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    auto& parent = frame_stack_.top();

    for (auto it = current.actions_.rbegin(); it != current.actions_.rend(); ++it)
        undo_action(*it);

    const size_t ultimate_mcts =
        get_ultimate_mcts_frame_depth_.get_ultimate_mcts_frame_depth();
    for (auto& rua : current.raised_units_) {
        if (rua.nand == nullptr || !(ultimate_mcts < rua.unit_boundary)) {
            co_yield rua.ultimate;
            parent.raised_units_.push_back(rua);
            continue;
        }
        rua.nand->armed_ = true;
        std::vector<const resolution_lineage*> yields;
        arm_catchup(rua.nand, yields);
        check_nand(rua.nand, yields);
        for (const resolution_lineage* y : yields)
            co_yield y;
    }
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
typename dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::factor*
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::intern_leaf(
    const resolution_lineage* rl) {
    const auto it = leaves_.find(rl);
    if (it != leaves_.end())
        return it->second;
    nodes_.emplace_back(1, nullptr, nullptr, rl);
    factor* f = &nodes_.back();
    leaves_.emplace(rl, f);
    return f;
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
typename dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::factor*
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::intern_pair(
    factor* left, factor* right) {
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

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
typename dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::factor*
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::build(
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

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
void dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::arm_catchup(
    factor* f, std::vector<const resolution_lineage*>& yields) {
    if (f->left_ == nullptr) {
        if (f->visited_ > 0)
            return;
        const auto chosen = try_get_chosen_goal_candidate_.try_get(f->leaf_rl_->parent);
        if (chosen && *chosen == f->leaf_rl_->idx)
            visit_leaf(f, yields);
        return;
    }
    arm_catchup(f->left_, yields);
    arm_catchup(f->right_, yields);
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
size_t dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::unvisited_count(
    const factor* f) const {
    if (f->left_ == nullptr)
        return (f->visited_ > 0) ? 0 : 1;
    return unvisited_count(f->left_) + unvisited_count(f->right_);
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
void dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::log_visit(
    factor* f, size_t amount) {
    frame_stack_.top().actions_.emplace_back(visit_delta{f, amount});
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
void dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::visit_leaf(
    factor* f, std::vector<const resolution_lineage*>& yields) {
    if (f->visited_ >= f->tuple_size_)
        return;
    f->visited_ += 1;
    log_visit(f, 1);
    after_increment(f, yields);
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
void dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::after_increment(
    factor* f, std::vector<const resolution_lineage*>& yields) {
    check_nand(f, yields);
    if (f->visited_ != f->tuple_size_) {
        for (factor* parent : f->parents_)
            check_nand(parent, yields);
        return;
    }
    for (factor* parent : f->parents_) {
        if (parent->visited_ >= parent->tuple_size_)
            continue;
        parent->visited_ += f->tuple_size_;
        log_visit(parent, f->tuple_size_);
        after_increment(parent, yields);
    }
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
void dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::check_nand(
    factor* f, std::vector<const resolution_lineage*>& yields) {
    if (!f->armed_)
        return;
    if (f->nand_multiplicity_ == 0)
        return;
    if (f->nand_fired_)
        return;
    if (unvisited_count(f) != 1)
        return;
    const resolution_lineage* remaining = unvisited_leaf(f);
    DEBUG_ASSERT(remaining != nullptr);
    for (size_t i = 0; i < f->nand_multiplicity_; ++i)
        yields.push_back(remaining);
    f->nand_fired_ = true;
    frame_stack_.top().actions_.emplace_back(nand_fired{f});
}


template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
const resolution_lineage*
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::unvisited_leaf(
    const factor* f) const {
    if (f->left_ == nullptr) {
        if (f->visited_ > 0)
            return nullptr;
        return f->leaf_rl_;
    }
    if (const resolution_lineage* u = unvisited_leaf(f->left_))
        return u;
    return unvisited_leaf(f->right_);
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
void dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::undo_action(
    const action& a) {
    if (const auto* d = std::get_if<visit_delta>(&a)) {
        DEBUG_ASSERT(d->node->visited_ >= d->amount);
        d->node->visited_ -= d->amount;
        return;
    }
    const auto& fired = std::get<nand_fired>(a);
    fired.node->nand_fired_ = false;
}

#endif
