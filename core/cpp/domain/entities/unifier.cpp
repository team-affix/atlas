#include "../../../hpp/domain/entities/unifier.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

unifier::unifier(const resolution_lineage* rl) :
    rl(rl),
    rep_changed_producer(locator::locate<i_event_producer<representative_changed_event>>()),
    unify_yielded_producer(locator::locate<i_event_producer<unify_yielded_event>>()),
    unify_failed_producer(locator::locate<i_event_producer<unify_failed_event>>()),
    unify_finished_producer(locator::locate<i_event_producer<unify_finished_event>>()) {}

const expr* unifier::whnf(const expr* key) {
    if (!std::holds_alternative<expr::var>(key->content))
        return key;

    const expr::var& var = std::get<expr::var>(key->content);
    auto it = bindings.find(var.index);
    if (it == bindings.end())
        return key;

    const expr* whnf_bound = whnf(it->second);
    bind(var.index, whnf_bound);
    return whnf_bound;
}

void unifier::push(const expr* lhs, const expr* rhs) {
    work_queue.push({lhs, rhs});
    unify_yielded();
}

void unifier::process_step() {
    auto [lhs, rhs] = work_queue.front();
    work_queue.pop();
    if (!process_pair(lhs, rhs)) { unify_failed_producer.produce({rl}); return; }
    if (!work_queue.empty()) { unify_yielded(); return; }
    unify_finished_producer.produce({rl});
}

void unifier::clear() {
    bindings.clear();
    work_queue = {};
}

bool unifier::process_pair(const expr* lhs, const expr* rhs) {
    lhs = whnf(lhs);
    rhs = whnf(rhs);

    const expr::var* lv = std::get_if<expr::var>(&lhs->content);
    const expr::var* rv = std::get_if<expr::var>(&rhs->content);

    if (lv && rv && lv->index == rv->index)
        return true;

    if (lv) {
        if (occurs_check(lv->index, rhs)) return false;
        bind(lv->index, rhs);
        rep_changed(lv->index);
        return true;
    }

    if (rv) {
        if (occurs_check(rv->index, lhs)) return false;
        bind(rv->index, lhs);
        rep_changed(rv->index);
        return true;
    }

    if (lhs->content.index() != rhs->content.index()) return false;

    if (std::holds_alternative<expr::functor>(lhs->content)) {
        const expr::functor& lf = std::get<expr::functor>(lhs->content);
        const expr::functor& rf = std::get<expr::functor>(rhs->content);
        if (lf.name != rf.name || lf.args.size() != rf.args.size()) return false;
        for (size_t i = 0; i < lf.args.size(); ++i)
            work_queue.push({lf.args[i], rf.args[i]});
    }

    return true;
}

bool unifier::occurs_check(uint32_t index, const expr* key) {
    key = whnf(key);
    if (const expr::var* v = std::get_if<expr::var>(&key->content))
        return v->index == index;
    if (const expr::functor* f = std::get_if<expr::functor>(&key->content))
        for (const expr* arg : f->args)
            if (occurs_check(index, arg))
                return true;
    return false;
}

void unifier::rep_changed(uint32_t index) {
    rep_changed_producer.produce(representative_changed_event{index});
}

void unifier::unify_yielded() {
    unify_yielded_producer.produce({rl});
}

void unifier::bind(uint32_t index, const expr* value) {
    auto it = bindings.find(index);
    if (it == bindings.end())
        bindings.insert({index, value});
    else if (it->second != value)
        it->second = value;
}
