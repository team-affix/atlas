#ifndef TOPIC_HPP
#define TOPIC_HPP

#include <queue>
#include <unordered_set>
#include "event_handler.hpp"
#include "task.hpp"
#include "scheduler.hpp"
#include "../bootstrap/resolver.hpp"

template <typename Event>
struct event_topic : task {
    event_topic(uint32_t);
    void produce(const Event&);
    void subscribe(event_handler<Event>&);
    void execute() override;
#ifndef DEBUG
private:
#endif
    scheduler& s;

    std::queue<Event> events;
    std::unordered_set<event_handler<Event>*> handlers;
};

template<typename Event>
event_topic<Event>::event_topic(uint32_t priority) : task(priority), s(resolver::resolve<scheduler>()) {
}

template <typename Event>
void event_topic<Event>::produce(const Event& event) {
    events.push(event);
    s.schedule(this);
}

template <typename Event>
void event_topic<Event>::subscribe(event_handler<Event>& handler) {
    handlers.insert(&handler);
}

template <typename Event>
void event_topic<Event>::execute() {
    const Event& event = events.front();
    for (auto* handler : handlers)
        handler->operator()(event);
    events.pop();
}

#endif
