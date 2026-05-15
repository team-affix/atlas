#include "../../hpp/utility/state_machine.hpp"
#include <exception>

// promise_type

state_machine state_machine::promise_type::get_return_object() {
    return state_machine{ std::coroutine_handle<promise_type>::from_promise(*this) };
}

std::suspend_always state_machine::promise_type::initial_suspend() { return {}; }

std::suspend_always state_machine::promise_type::final_suspend() noexcept { return {}; }

void state_machine::promise_type::return_void() {}

void state_machine::promise_type::unhandled_exception() { std::terminate(); }

// state_machine

state_machine::state_machine(std::coroutine_handle<promise_type> h) : handle_(h) {}

state_machine::~state_machine() {
    if (handle_) handle_.destroy();
}

state_machine::state_machine(state_machine&& other) noexcept
    : handle_(other.handle_) {
    other.handle_ = nullptr;
}

state_machine& state_machine::operator=(state_machine&& other) noexcept {
    if (this != &other) {
        if (handle_) handle_.destroy();
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }
    return *this;
}

void state_machine::resume() { handle_.resume(); }

bool state_machine::done() const { return handle_.done(); }
