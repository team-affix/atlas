#include "infrastructure/lp_cdcl_elimination_generator.hpp"

#include <utility>
#include <variant>
#include "debug_assert.hpp"

namespace {

bool members_contain(const std::vector<const resolution_lineage*>& members,
                     const resolution_lineage* rl) {
    for (const resolution_lineage* member : members)
        if (member == rl)
            return true;
    return false;
}

} // namespace

lp_cdcl_elimination_generator::lp_cdcl_elimination_generator()
    : frames_()
    , current_frame_id_()
    , current_frame_(&frames_[current_frame_id_])
    , pending_eliminations_()
    , next_avoidance_id_(0)
    , next_timestamp_(0) {}

// The whole decision set, not a lemma: an ancestor decision that later takes a
// different rule satisfies this avoidance, and only a full-set member names it.
void lp_cdcl_elimination_generator::learn() {
    if (current_frame_id_.empty()) return;

    const avoidance_id id = next_avoidance_id_++;
    lp_avoidance learned{id, {current_frame_id_.begin(), current_frame_id_.end()}};
    frames_[lp_decision_frame_id{}].mailbox[id] =
        lp_avoidance_content_update{std::move(learned)};
}

// Delivery only. Everything the child does with this arrives in enter().
void lp_cdcl_elimination_generator::descend(const resolution_lineage* rl) {
    lp_decision_frame_id child_id = current_frame_id_;
    child_id.insert(rl);

    // Insert the child first. frames_[] may rehash, which would dangle any
    // pointer we already held into the table — including current_frame_.
    frames_[child_id];
    lp_decision_frame& parent = frames_.at(current_frame_id_);
    lp_decision_frame& child = frames_.at(child_id);

    for (auto it = parent.change_log.lower_bound(parent.continuations[rl]);
         it != parent.change_log.end(); ++it)
        child.mailbox[it->second] = parent.avoidances.at(it->second);
    // next_timestamp_ is monotonic and global, so this covers the empty-log case
    // without a sentinel: everything stamped later is necessarily above it.
    parent.continuations[rl] = next_timestamp_;

    current_frame_id_ = std::move(child_id);
    current_frame_ = &child;
}

void lp_cdcl_elimination_generator::enter() {
    for (const auto& [id, incoming] : current_frame_->mailbox)
        merge(id, incoming);
    current_frame_->mailbox.clear();

    // The whole set, not just what arrived: tear-down undid these eliminations
    // even though the proofs behind them stand.
    for (const resolution_lineage* elim : current_frame_->forced)
        pending_eliminations_.push_back(elim);
}

void lp_cdcl_elimination_generator::merge(avoidance_id id,
                                          const lp_avoidance_update& incoming) {
    const auto held = current_frame_->avoidances.find(id);
    const bool is_held = held != current_frame_->avoidances.end();

    if (std::get_if<lp_avoidance_satisfied>(&incoming) != nullptr) {
        if (is_held && std::get_if<lp_avoidance_satisfied>(&held->second) != nullptr)
            return; // already known dead; re-recording would churn the log forever
        if (is_held) {
            const auto* content = std::get_if<lp_avoidance_content_update>(&held->second);
            if (content != nullptr && content->avoidance.members.size() == 1)
                current_frame_->forced.erase(content->avoidance.members.front());
            unarm(id);
        }
        record_change(id, lp_avoidance_satisfied{});
        return;
    }

    const lp_avoidance& delivered =
        std::get_if<lp_avoidance_content_update>(&incoming)->avoidance;

    if (!is_held) {
        adopt(delivered);
        return;
    }
    if (std::get_if<lp_avoidance_satisfied>(&held->second) != nullptr)
        return; // satisfaction is terminal, and valid in every descendant

    // Intersect rather than take the shorter side. Each frame reduced by
    // resolutions the other never saw -- the parent by its own units, this frame
    // by everything under its extra decisions -- so neither member set is
    // reliably a subset of the other, and a member this frame keeps but the
    // parent dropped is one whose goal already resolved above us.
    const lp_avoidance& mine =
        std::get_if<lp_avoidance_content_update>(&held->second)->avoidance;
    lp_avoidance merged{id, {}};
    for (const resolution_lineage* member : mine.members)
        if (members_contain(delivered.members, member))
            merged.members.push_back(member);

    DEBUG_ASSERT(!merged.members.empty());
    if (merged.members.size() == mine.members.size()) return;
    unarm(id);
    adopt(std::move(merged));
}

// A one-member avoidance is a forced elimination, and is stored rather than
// dropped so children reached by another edge still learn the survivor.
void lp_cdcl_elimination_generator::adopt(lp_avoidance reduced) {
    DEBUG_ASSERT(!reduced.members.empty());

    const avoidance_id id = reduced.id;
    const bool is_unit = reduced.members.size() == 1;
    if (is_unit) current_frame_->forced.insert(reduced.members.front());

    record_change(id, lp_avoidance_content_update{std::move(reduced)});
    if (!is_unit) arm(id);
}

// The single write path, which is what makes changes cascade to grandchildren
// with no extra bookkeeping: the old log entry goes so the log stays at one
// entry per avoidance rather than one per update.
void lp_cdcl_elimination_generator::record_change(avoidance_id id,
                                                  lp_avoidance_update update) {
    const auto previous = current_frame_->timestamps.find(id);
    if (previous != current_frame_->timestamps.end())
        current_frame_->change_log.erase(previous->second);

    const lp_timestamp now = next_timestamp_++;
    current_frame_->change_log.insert({now, id});
    current_frame_->timestamps[id] = now;
    current_frame_->avoidances[id] = std::move(update);
}

void lp_cdcl_elimination_generator::arm(avoidance_id id) {
    const lp_avoidance& armed =
        std::get_if<lp_avoidance_content_update>(&current_frame_->avoidances.at(id))
            ->avoidance;
    for (const resolution_lineage* member : armed.members)
        current_frame_->watched_goals[member->parent].insert(id);
}

void lp_cdcl_elimination_generator::unarm(avoidance_id id) { // mirror image of arm
    const auto* content =
        std::get_if<lp_avoidance_content_update>(&current_frame_->avoidances.at(id));
    if (content == nullptr) return;
    for (const resolution_lineage* member : content->avoidance.members) {
        const auto watchers = current_frame_->watched_goals.find(member->parent);
        if (watchers != current_frame_->watched_goals.end())
            watchers->second.erase(id);
    }
}

coroutine<const resolution_lineage*, void> lp_cdcl_elimination_generator::flush() {
    std::vector<const resolution_lineage*> staged;
    staged.swap(pending_eliminations_);
    for (const resolution_lineage* elim : staged)
        co_yield elim;
}

// The reduction step. Correct by induction: this frame's copy is already reduced
// by every decision above rl, so rl is the one new resolution to apply.
coroutine<const resolution_lineage*, void>
lp_cdcl_elimination_generator::constrain(const resolution_lineage* rl) {
    // Same-parent leftovers are the resolver's: run_sim resolve() already
    // deactivates every remaining candidate of rl->parent. Yielding one would
    // unit-push that goal and then erase its rule bucket.
    std::vector<const resolution_lineage*> staged;
    staged.swap(pending_eliminations_);
    for (const resolution_lineage* elim : staged)
        if (elim->parent != rl->parent)
            co_yield elim;

    const auto watchers = current_frame_->watched_goals.find(rl->parent);
    if (watchers == current_frame_->watched_goals.end()) co_return;

    const std::vector<avoidance_id> armed(watchers->second.begin(),
                                          watchers->second.end());
    for (const avoidance_id id : armed) {
        const auto* content =
            std::get_if<lp_avoidance_content_update>(&current_frame_->avoidances.at(id));
        if (content == nullptr) continue;

        const std::vector<const resolution_lineage*>& members =
            content->avoidance.members;
        size_t at = members.size();
        for (size_t i = 0; i < members.size(); ++i)
            if (members.at(i)->parent == rl->parent) {
                at = i;
                break;
            }
        if (at == members.size()) continue; // already trimmed earlier this pass

        if (members.at(at)->idx != rl->idx) {
            unarm(id);
            record_change(id, lp_avoidance_satisfied{});
            continue;
        }

        lp_avoidance reduced{id, members};
        reduced.members.erase(reduced.members.begin() + static_cast<long>(at));
        const bool is_unit = reduced.members.size() == 1;
        const resolution_lineage* survivor = is_unit ? reduced.members.front() : nullptr;

        unarm(id);
        adopt(std::move(reduced));
        if (is_unit && survivor->parent != rl->parent)
            co_yield survivor;
    }
}

void lp_cdcl_elimination_generator::cleanup() {
    current_frame_id_.clear();
    current_frame_ = &frames_[current_frame_id_];
    pending_eliminations_.clear();
}
