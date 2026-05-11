#ifndef TOPIC_HPP
#define TOPIC_HPP

#include <queue>
#include <unordered_set>
#include "event_handler.hpp"
#include "task.hpp"
#include "scheduler.hpp"
#include "../bootstrap/locator.hpp"
#include "../domain/interfaces/i_logger.hpp"
#include "../domain/interfaces/i_event_producer.hpp"

template <typename Event>
struct event_topic : i_event_producer<Event>, task {
    event_topic(uint32_t);
    void produce(const Event&) override;
    void subscribe(event_handler<Event>&);
    void execute() override;
private:
    scheduler& s;
    i_logger& logger;

    std::queue<Event> events;
    std::unordered_set<event_handler<Event>*> handlers;
};

template<typename Event>
event_topic<Event>::event_topic(uint32_t priority) :
    task(priority),
    s(locator::resolve<scheduler>()),
    logger(locator::resolve<i_logger>()) {
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
#ifdef DEBUG
    logger.get_ostream() << event;
#endif
    for (auto* handler : handlers)
        handler->handle(event);
    events.pop();
}

#endif
