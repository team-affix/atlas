#ifndef COMMAND_TOPIC_HPP
#define COMMAND_TOPIC_HPP

#include <queue>
#include "command_handler.hpp"

template<typename Command>
struct command_topic {
    void produce(const Command&);
    void subscribe(command_handler<Command>&);
#ifndef DEBUG
private:
#endif
    std::queue<Command> commands;
    command_handler<Command>* handler;
};

template<typename Command>
void command_topic<Command>::produce(const Command& command) {
    handler->operator()(command);
}

template<typename Command>
void command_topic<Command>::subscribe(command_handler<Command>& handler) {
    this->handler = &handler;
}

#endif
