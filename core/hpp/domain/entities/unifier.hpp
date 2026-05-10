#ifndef UNIFIER_HPP
#define UNIFIER_HPP

#include <unordered_map>
#include <queue>
#include <utility>
#include "../interfaces/i_unifier.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../value_objects/expr.hpp"
#include "../value_objects/lineage.hpp"
#include "../events/representative_changed_event.hpp"
#include "../events/unify_resuming_event.hpp"
#include "../events/unify_yielded_event.hpp"
#include "../events/unify_failed_event.hpp"
#include "../events/unify_finished_event.hpp"

struct unifier : i_unifier {
    explicit unifier(const resolution_lineage* rl);
    const expr* whnf(const expr*) override;
    void push(const expr*, const expr*) override;
    void process_step() override;
    void clear() override;
private:
    bool occurs_check(uint32_t, const expr*);
    void bind(uint32_t, const expr*);
    void process_pair(const expr*, const expr*);

    const resolution_lineage* rl;
    std::unordered_map<uint32_t, const expr*> bindings;
    std::queue<std::pair<const expr*, const expr*>> work_queue;

    i_event_producer<representative_changed_event>& rep_changed_producer;
    i_event_producer<unify_resuming_event>& unify_resuming_producer;
    i_event_producer<unify_yielded_event>& unify_yielded_producer;
    i_event_producer<unify_failed_event>& unify_failed_producer;
    i_event_producer<unify_finished_event>& unify_finished_producer;
};

#endif
