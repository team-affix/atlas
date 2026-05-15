#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include <coroutine>

struct state_machine {
    struct promise_type {
        state_machine get_return_object();
        std::suspend_always initial_suspend();
        std::suspend_always final_suspend() noexcept;
        void return_void();
        void unhandled_exception();
    };

    explicit state_machine(std::coroutine_handle<promise_type>);
    ~state_machine();
    state_machine(const state_machine&) = delete;
    state_machine& operator=(const state_machine&) = delete;
    state_machine(state_machine&&) noexcept;
    state_machine& operator=(state_machine&&) noexcept;

    void resume();
    bool done() const;

private:
    std::coroutine_handle<promise_type> handle_;
};

#endif
