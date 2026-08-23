#include "infrastructure/lp_cdcl_elimination_generator.hpp"

#include <utility>
#include "debug_assert.hpp"

lp_cdcl_elimination_generator::lp_cdcl_elimination_generator()
    : next_avoidance_id_(0) {}

std::optional<const resolution_lineage*> lp_cdcl_elimination_generator::learn() {
    // current_frame_id_ is the decision set: descend() has been accumulating it
    // one decision at a time since the sim began.
    if (current_frame_id_.empty())
        return std::nullopt;
    if (current_frame_id_.size() == 1)
        return *current_frame_id_.begin();

    lp_avoidance av{next_avoidance_id_++,
                    {current_frame_id_.begin(), current_frame_id_.end()}};
    lp_decision_frame& root = frames_[lp_decision_frame_id{}];
    root.received.insert(av.id);
    root.cache.push_back(std::move(av));
    arm(root, root.cache.size() - 1);
    return std::nullopt;
}

void lp_cdcl_elimination_generator::descend(const resolution_lineage* rl) {
    lp_decision_frame_id child_id = current_frame_id_;
    child_id.insert(rl);

    // Holding both references is safe: unordered_map only invalidates references
    // to an element when that element is erased, and frames are never erased.
    lp_decision_frame& parent = frames_[current_frame_id_];
    lp_decision_frame& child = frames_[child_id];

    const size_t end = parent.cache.size();
    for (size_t pos = parent.continuations[rl]; pos < end; ++pos) {
        const lp_avoidance& entry = parent.cache.at(pos);
        if (entry.members.empty())
            continue;
        if (child.received.contains(entry.id))
            continue;

        // The parent cache is already exact for the parent, so this edge carries
        // exactly one piece of new information: rl was taken.
        lp_avoidance reduced{entry.id, {}};
        reduced.members.reserve(entry.members.size());
        bool trimmed = false;
        for (const resolution_lineage* member : entry.members) {
            if (member == rl)
                continue;
            if (member->parent == rl->parent) {
                trimmed = true;
                break;
            }
            reduced.members.push_back(member);
        }
        if (trimmed)
            continue;

        DEBUG_ASSERT(!reduced.members.empty());
        if (reduced.members.size() == 1) {
            child.forced.insert(reduced.members.front());
            child.received.insert(reduced.id);
            continue;
        }
        receive(child, std::move(reduced));
    }
    parent.continuations[rl] = end;

    // Re-apply everything this child has ever proven, not just what this edge
    // just derived. Only the child's own set is needed: reaching it means the
    // sim descended through each ancestor frame, and each re-applied its own.
    for (const resolution_lineage* elim : child.forced)
        pending_eliminations_.push_back(elim);
    current_frame_id_ = std::move(child_id);
}

coroutine<const resolution_lineage*, void>
lp_cdcl_elimination_generator::constrain(const resolution_lineage* rl) {
    // Drain what the descent onto this edge already forced. Taken by swap so the
    // buffer is empty even if the caller abandons the coroutine mid-drain.
    std::vector<const resolution_lineage*> drained;
    drained.swap(pending_eliminations_);
    for (const resolution_lineage* elim : drained)
        co_yield elim;

    lp_decision_frame& frame = frames_[current_frame_id_];
    const auto watchers = frame.watched_goals.find(rl->parent);
    if (watchers == frame.watched_goals.end())
        co_return;

    // Copied because the body below mutates this goal's watcher set. For a
    // decision resolution this loop is a provable no-op: descend() already
    // erased or trimmed every member on rl->parent.
    const std::vector<size_t> positions(watchers->second.begin(), watchers->second.end());
    for (const size_t pos : positions) {
        lp_avoidance& entry = frame.cache.at(pos);
        if (entry.members.empty())
            continue;

        size_t member_pos = entry.members.size();
        for (size_t i = 0; i < entry.members.size(); ++i) {
            if (entry.members.at(i)->parent == rl->parent) {
                member_pos = i;
                break;
            }
        }
        DEBUG_ASSERT(member_pos < entry.members.size());

        // The goal took a different rule, so this avoidance can never complete.
        if (entry.members.at(member_pos)->idx != rl->idx) {
            tombstone(frame, pos);
            continue;
        }
        // Reducing would leave a single member, which is a forced elimination
        // rather than something to cache. Tombstone first so every member is
        // still present to unwatch.
        if (entry.members.size() == 2) {
            const resolution_lineage* survivor = entry.members.at(member_pos == 0 ? 1 : 0);
            tombstone(frame, pos);
            frame.forced.insert(survivor);
            co_yield survivor;
            continue;
        }
        entry.members.erase(entry.members.begin() + member_pos);
        frame.watched_goals.at(rl->parent).erase(pos);
    }
}

void lp_cdcl_elimination_generator::cleanup() {
    // frames_ deliberately survives: it is the cross-sim memory.
    current_frame_id_.clear();
    pending_eliminations_.clear();
}

void lp_cdcl_elimination_generator::receive(lp_decision_frame& child, lp_avoidance reduced) {
    child.received.insert(reduced.id);
    child.cache.push_back(std::move(reduced));
    arm(child, child.cache.size() - 1);
}

void lp_cdcl_elimination_generator::arm(lp_decision_frame& f, size_t pos) {
    for (const resolution_lineage* member : f.cache.at(pos).members)
        f.watched_goals[member->parent].insert(pos);
}

void lp_cdcl_elimination_generator::tombstone(lp_decision_frame& f, size_t pos) { // mirror image of arm
    for (const resolution_lineage* member : f.cache.at(pos).members) {
        const auto watchers = f.watched_goals.find(member->parent);
        if (watchers != f.watched_goals.end())
            watchers->second.erase(pos);
    }
    f.cache.at(pos).members.clear();
}
