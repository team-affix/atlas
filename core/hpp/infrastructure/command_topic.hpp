#ifndef COMMAND_TOPIC_HPP
#define COMMAND_TOPIC_HPP

#include <queue>
#include "command_handler.hpp"
#include "task.hpp"
#include "scheduler.hpp"
#include "../bootstrap/resolver.hpp"

template<typename Command>
struct command_topic : task {
    command_topic(uint32_t);
    void produce(const Command&);
    void subscribe(command_handler<Command>&);
    void execute() override;
#ifndef DEBUG
private:
#endif
    scheduler& s;

    std::queue<Command> commands;
    command_handler<Command>* handler;
};

template<typename Command>
command_topic<Command>::command_topic(uint32_t priority) : task(priority), s(resolver::resolve<scheduler>()) {
}

template<typename Command>
void command_topic<Command>::produce(const Command& command) {
    commands.push(command);
    s.schedule(this);
}

template<typename Command>
void command_topic<Command>::subscribe(command_handler<Command>& handler) {
    this->handler = &handler;
}

template<typename Command>
void command_topic<Command>::execute() {
    handler->operator()(commands.front());
    commands.pop();
}

#endif
