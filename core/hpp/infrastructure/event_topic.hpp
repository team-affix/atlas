#ifndef TOPIC_HPP
#define TOPIC_HPP

#include <queue>
#include <unordered_set>
#include "event_handler.hpp"

template <typename Event>
struct event_topic {
    void produce(const Event&);
    void subscribe(event_handler<Event>&);
#ifndef DEBUG
private:
#endif
    std::queue<Event> events;
    std::unordered_set<event_handler<Event>*> handlers;
};

template <typename Event>
void event_topic<Event>::produce(const Event& event) {
    for (auto& handler : handlers)
        handler->operator()(event);
}

template <typename Event>
void event_topic<Event>::subscribe(event_handler<Event>& handler) {
    handlers.insert(&handler);
}

#endif
