#ifndef DBUCT_BT_CDCL_ELIMINATION_GENERATOR_HPP
#define DBUCT_BT_CDCL_ELIMINATION_GENERATOR_HPP

#include <algorithm>
#include <cstddef>
#include <deque>
#include <list>
#include <stack>
#include <unordered_map>
#include <variant>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/bt_cdcl_action.hpp"
#include "value_objects/bt_cdcl_factor.hpp"
#include "value_objects/bt_cdcl_nand_fired.hpp"
#include "value_objects/bt_cdcl_pair_key.hpp"
#include "value_objects/bt_cdcl_pair_key_hash.hpp"
#include "value_objects/bt_cdcl_visit_delta.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/raised_nand.hpp"
#include "value_objects/resolution_lineage_ptr_less.hpp"
#include "debug_assert.hpp"

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
    struct frame {
        std::list<bt_cdcl_action> actions_;
        std::list<raised_nand> raised_nands_;
    };

    bt_cdcl_factor* intern_leaf(const resolution_lineage*);
    bt_cdcl_factor* intern_pair(bt_cdcl_factor*, bt_cdcl_factor*);
    bt_cdcl_factor* intern_members(const std::vector<const resolution_lineage*>&, size_t, size_t);
    coroutine<const resolution_lineage*, void> visit_leaf(bt_cdcl_factor*);
    coroutine<const resolution_lineage*, void> propagate_visit(bt_cdcl_factor*);
    coroutine<const resolution_lineage*, void> try_fire(bt_cdcl_factor*);
    const resolution_lineage* find_unvisited_leaf(const bt_cdcl_factor*, size_t&) const;
    coroutine<const resolution_lineage*, void> visit_chosen_leaves(bt_cdcl_factor*);
    void log(bt_cdcl_action);
    void undo_action(const bt_cdcl_action&);

    std::deque<bt_cdcl_factor> nodes_;
    std::unordered_map<const resolution_lineage*, bt_cdcl_factor*> leaves_;
    std::unordered_map<bt_cdcl_pair_key, bt_cdcl_factor*, bt_cdcl_pair_key_hash> pairs_;
    std::stack<frame> frame_stack_;

    ITryGetChosenGoalCandidate& try_get_chosen_goal_candidate_;
    IGetPenultimateMctsFrameDepth& get_penultimate_mcts_frame_depth_;
    IDeriveDecisionLemma& derive_decision_lemma_;
    IGetUltimateDecision& get_ultimate_decision_;
    IGetUltimateMctsFrameDepth& get_ultimate_mcts_frame_depth_;
};

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
void
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::learn() {
    lemma l = derive_decision_lemma_.derive_decision_lemma();
    const auto& resolutions = l.get_resolutions();
    if (resolutions.empty())
        return;

    if (resolutions.size() == 1) {
        frame_stack_.top().raised_nands_.push_back(raised_nand{
            nullptr, 0, *resolutions.begin()});
        return;
    }

    const size_t unit_boundary =
        get_penultimate_mcts_frame_depth_.get_penultimate_mcts_frame_depth();
    const resolution_lineage* ultimate = get_ultimate_decision_.get_ultimate_decision();

    std::vector<const resolution_lineage*> members(resolutions.begin(), resolutions.end());
    std::sort(members.begin(), members.end(), resolution_lineage_ptr_less{});
    bt_cdcl_factor* root = intern_members(members, 0, members.size());
    root->nand_multiplicity += 1;
    frame_stack_.top().raised_nands_.push_back(raised_nand{root, unit_boundary, ultimate});
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
coroutine<const resolution_lineage*, void>
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::constrain(
    const resolution_lineage* rl) {
    const auto it = leaves_.find(rl);
    if (it == leaves_.end())
        co_return;
    auto coro = visit_leaf(it->second);
    while (auto lineage = coro.next())
        co_yield *lineage;
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
    for (auto& rn : current.raised_nands_) {
        if (rn.nand == nullptr || ultimate_mcts >= rn.unit_boundary) {
            co_yield rn.ultimate;
            parent.raised_nands_.push_back(rn);
            continue;
        }
        rn.nand->armed = true;
        auto chosen_leaves = visit_chosen_leaves(rn.nand);
        while (auto lineage = chosen_leaves.next())
            co_yield *lineage;
        auto fire = try_fire(rn.nand);
        while (auto lineage = fire.next())
            co_yield *lineage;
    }
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
bt_cdcl_factor*
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::intern_leaf(
    const resolution_lineage* rl) {
    const auto it = leaves_.find(rl);
    if (it != leaves_.end())
        return it->second;
    nodes_.emplace_back(1, nullptr, nullptr, rl);
    bt_cdcl_factor* f = &nodes_.back();
    leaves_.emplace(rl, f);
    return f;
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
bt_cdcl_factor*
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::intern_pair(
    bt_cdcl_factor* left, bt_cdcl_factor* right) {
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

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
bt_cdcl_factor*
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::intern_members(
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

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
coroutine<const resolution_lineage*, void>
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::visit_leaf(
    bt_cdcl_factor* f) {
    if (f->visited >= f->tuple_size)
        co_return;
    f->visited += 1;
    log(bt_cdcl_visit_delta{f, 1});
    auto coro = propagate_visit(f);
    while (auto lineage = coro.next())
        co_yield *lineage;
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
coroutine<const resolution_lineage*, void>
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::propagate_visit(
    bt_cdcl_factor* f) {
    auto self_fire = try_fire(f);
    while (auto lineage = self_fire.next())
        co_yield *lineage;
    for (bt_cdcl_factor* parent : f->parents) {
        if (f->visited != f->tuple_size) {
            auto parent_fire = try_fire(parent);
            while (auto lineage = parent_fire.next())
                co_yield *lineage;
            continue;
        }
        if (parent->visited >= parent->tuple_size)
            continue;
        parent->visited += f->tuple_size;
        log(bt_cdcl_visit_delta{parent, f->tuple_size});
        auto prop = propagate_visit(parent);
        while (auto lineage = prop.next())
            co_yield *lineage;
    }
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
coroutine<const resolution_lineage*, void>
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::try_fire(
    bt_cdcl_factor* f) {
    if (!f->armed)
        co_return;
    if (f->nand_multiplicity == 0)
        co_return;
    if (f->nand_fired)
        co_return;
    size_t count = 0;
    const resolution_lineage* remaining = find_unvisited_leaf(f, count);
    if (count != 1)
        co_return;
    f->nand_fired = true;
    log(bt_cdcl_nand_fired{f});
    for (size_t i = 0; i < f->nand_multiplicity; ++i)
        co_yield remaining;
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
const resolution_lineage*
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::find_unvisited_leaf(
    const bt_cdcl_factor* f, size_t& count) const {
    if (count > 1)
        return nullptr;
    if (f->left == nullptr) {
        if (f->visited == 0)
            ++count;
        return (count == 1) ? f->leaf_rl : nullptr;
    }
    const resolution_lineage* lu = find_unvisited_leaf(f->left, count);
    const resolution_lineage* ru = find_unvisited_leaf(f->right, count);
    return lu ? lu : ru;
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
coroutine<const resolution_lineage*, void>
dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::visit_chosen_leaves(
    bt_cdcl_factor* f) {
    if (f->left != nullptr) {
        auto left_coro = visit_chosen_leaves(f->left);
        while (auto lineage = left_coro.next())
            co_yield *lineage;
        auto right_coro = visit_chosen_leaves(f->right);
        while (auto lineage = right_coro.next())
            co_yield *lineage;
        co_return;
    }
    if (f->visited > 0)
        co_return;
    const auto chosen = try_get_chosen_goal_candidate_.try_get(f->leaf_rl->parent);
    if (!chosen || *chosen != f->leaf_rl->idx)
        co_return;
    auto leaf_coro = visit_leaf(f);
    while (auto lineage = leaf_coro.next())
        co_yield *lineage;
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
void dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::log(
    bt_cdcl_action action) {
    frame_stack_.top().actions_.emplace_back(action);
}

template<typename ITGCC, typename IGPMFD, typename IDL, typename IGUD, typename IGUMFD>
void dbuct_bt_cdcl_elimination_generator<ITGCC, IGPMFD, IDL, IGUD, IGUMFD>::undo_action(
    const bt_cdcl_action& a) {
    if (const auto* d = std::get_if<bt_cdcl_visit_delta>(&a)) {
        DEBUG_ASSERT(d->node->visited >= d->amount);
        d->node->visited -= d->amount;
        return;
    }
    const auto& fired = std::get<bt_cdcl_nand_fired>(a);
    fired.node->nand_fired = false;
}

#endif
