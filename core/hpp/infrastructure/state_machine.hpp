#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include <coroutine>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <utility>

template<typename Yield, typename Return>
struct state_machine {
    static_assert(
        !std::is_void_v<Yield> && !std::is_void_v<Return>,
        "state_machine<Yield, Return> requires non-void Yield and Return; "
        "use state_machine<Yield, void>, state_machine<void, Return>, or state_machine<void, void>");
};

template<typename Promise>
struct state_machine_ops {
    explicit state_machine_ops(std::coroutine_handle<Promise> h) : handle_(h) {}

    ~state_machine_ops() {
        if (handle_)
            handle_.destroy();
    }

    state_machine_ops(const state_machine_ops&) = delete;
    state_machine_ops& operator=(const state_machine_ops&) = delete;

    state_machine_ops(state_machine_ops&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    state_machine_ops& operator=(state_machine_ops&& other) noexcept {
        if (this != &other) {
            if (handle_)
                handle_.destroy();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    void resume() {
        if (handle_.promise().exception_)
            std::rethrow_exception(handle_.promise().exception_);
        if (handle_.done())
            return;
        handle_.promise().at_yield_ = false;
        handle_.resume();
        if (handle_.promise().exception_)
            std::rethrow_exception(handle_.promise().exception_);
    }

    bool done() const { return !handle_ || handle_.done(); }

    bool has_yield() const { return handle_ && handle_.promise().at_yield_; }

    Promise& promise() { return handle_.promise(); }

    const Promise& promise() const { return handle_.promise(); }

private:
    std::coroutine_handle<Promise> handle_{};
};

// --- state_machine<Yield, Return> : value yields, value return ---

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
struct state_machine<Yield, Return> {
    struct promise_type {
        state_machine get_return_object() {
            return state_machine{
                std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        void unhandled_exception() { exception_ = std::current_exception(); }

        std::suspend_always yield_value(Yield v) {
            yield_ = std::move(v);
            at_yield_ = true;
            return {};
        }

        void return_value(Return r) {
            return_ = std::move(r);
            return_ready_ = true;
            at_yield_ = false;
            exception_ = nullptr;
        }

        Yield yield_{};
        Return return_{};
        bool return_ready_{false};
        bool at_yield_{false};
        std::exception_ptr exception_{};
    };

    explicit state_machine(std::coroutine_handle<promise_type> h) : ops_(h) {}

    state_machine(state_machine&&) = default;
    state_machine& operator=(state_machine&&) = default;
    state_machine(const state_machine&) = delete;
    state_machine& operator=(const state_machine&) = delete;

    void resume() { ops_.resume(); }
    bool done() const { return ops_.done(); }
    bool has_yield() const { return ops_.has_yield(); }

    Yield consume_yield() {
        if (!has_yield())
            throw std::logic_error("state_machine::consume_yield without yield");
        auto v = std::move(ops_.promise().yield_);
        ops_.promise().at_yield_ = false;
        return v;
    }

    Return result() const {
        if (!done() || !ops_.promise().return_ready_)
            throw std::logic_error("state_machine::result before completion");
        return ops_.promise().return_;
    }

private:
    state_machine_ops<promise_type> ops_;
};

// --- state_machine<Yield, void> : value yields, void return ---

template<typename Yield>
    requires(!std::is_void_v<Yield>)
struct state_machine<Yield, void> {
    struct promise_type {
        state_machine get_return_object() {
            return state_machine{
                std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        void unhandled_exception() { exception_ = std::current_exception(); }

        std::suspend_always yield_value(Yield v) {
            yield_ = std::move(v);
            at_yield_ = true;
            return {};
        }

        void return_void() {
            at_yield_ = false;
            exception_ = nullptr;
        }

        Yield yield_{};
        bool at_yield_{false};
        std::exception_ptr exception_{};
    };

    explicit state_machine(std::coroutine_handle<promise_type> h) : ops_(h) {}

    state_machine(state_machine&&) = default;
    state_machine& operator=(state_machine&&) = default;
    state_machine(const state_machine&) = delete;
    state_machine& operator=(const state_machine&) = delete;

    void resume() { ops_.resume(); }
    bool done() const { return ops_.done(); }
    bool has_yield() const { return ops_.has_yield(); }

    Yield consume_yield() {
        if (!has_yield())
            throw std::logic_error("state_machine::consume_yield without yield");
        auto v = std::move(ops_.promise().yield_);
        ops_.promise().at_yield_ = false;
        return v;
    }

private:
    state_machine_ops<promise_type> ops_;
};

// --- state_machine<void, Return> : no yields; suspend with co_await std::suspend_always{} ---

template<typename Return>
    requires(!std::is_void_v<Return>)
struct state_machine<void, Return> {
    struct promise_type {
        state_machine get_return_object() {
            return state_machine{
                std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        void unhandled_exception() { exception_ = std::current_exception(); }

        void return_value(Return r) {
            return_ = std::move(r);
            return_ready_ = true;
            exception_ = nullptr;
        }

        Return return_{};
        bool return_ready_{false};
        bool at_yield_{false};
        std::exception_ptr exception_{};
    };

    explicit state_machine(std::coroutine_handle<promise_type> h) : ops_(h) {}

    state_machine(state_machine&&) = default;
    state_machine& operator=(state_machine&&) = default;
    state_machine(const state_machine&) = delete;
    state_machine& operator=(const state_machine&) = delete;

    void resume() { ops_.resume(); }
    bool done() const { return ops_.done(); }

    Return result() const {
        if (!done() || !ops_.promise().return_ready_)
            throw std::logic_error("state_machine::result before completion");
        return ops_.promise().return_;
    }

private:
    state_machine_ops<promise_type> ops_;
};

// --- state_machine<void, void> : no yields; suspend with co_await std::suspend_always{} ---

template<>
struct state_machine<void, void> {
    struct promise_type {
        state_machine get_return_object() {
            return state_machine{
                std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        void unhandled_exception() { exception_ = std::current_exception(); }

        void return_void() { exception_ = nullptr; }

        bool at_yield_{false};
        std::exception_ptr exception_{};
    };

    explicit state_machine(std::coroutine_handle<promise_type> h) : ops_(h) {}

    state_machine(state_machine&&) = default;
    state_machine& operator=(state_machine&&) = default;
    state_machine(const state_machine&) = delete;
    state_machine& operator=(const state_machine&) = delete;

    void resume() { ops_.resume(); }
    bool done() const { return ops_.done(); }

private:
    state_machine_ops<promise_type> ops_;
};

#endif
