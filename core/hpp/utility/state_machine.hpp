#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include <coroutine>
#include <exception>
#include <optional>
#include <stdexcept>
#include <utility>

template<typename T>
struct state_machine {
    struct promise_type {
        state_machine<T> get_return_object() {
            return state_machine<T>{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() { last_yield_.reset(); }
        void unhandled_exception() { std::terminate(); }
        std::suspend_always yield_value(const T& v) {
            last_yield_ = v;
            return {};
        }
        std::suspend_always yield_value(T&& v) {
            last_yield_ = std::move(v);
            return {};
        }
        std::optional<T> last_yield_{};
    };

    explicit state_machine(std::coroutine_handle<promise_type> h) : handle_(h) {}

    ~state_machine() {
        if (handle_)
            handle_.destroy();
    }

    state_machine(const state_machine&) = delete;
    state_machine& operator=(const state_machine&) = delete;

    state_machine(state_machine&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    state_machine& operator=(state_machine&& other) noexcept {
        if (this != &other) {
            if (handle_)
                handle_.destroy();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    T resume() {
        handle_.resume();
        return std::move(*handle_.promise().last_yield_);
    }

    bool done() const { return handle_.done(); }

private:
    std::coroutine_handle<promise_type> handle_{};
};

template<>
struct state_machine<void> {
    struct promise_type {
        state_machine<void> get_return_object() {
            return state_machine<void>{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    explicit state_machine(std::coroutine_handle<promise_type> h) : handle_(h) {}

    ~state_machine() {
        if (handle_)
            handle_.destroy();
    }

    state_machine(const state_machine&) = delete;
    state_machine& operator=(const state_machine&) = delete;

    state_machine(state_machine&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    state_machine& operator=(state_machine&& other) noexcept {
        if (this != &other) {
            if (handle_)
                handle_.destroy();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    void resume() { handle_.resume(); }

    bool done() const { return handle_.done(); }

private:
    std::coroutine_handle<promise_type> handle_{};
};

#endif
