#ifndef COROUTINE_HPP
#define COROUTINE_HPP

#include <coroutine>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <utility>

template<typename Promise>
struct coroutine_ops {
    explicit coroutine_ops(std::coroutine_handle<Promise> h) : handle_(h) {}

    ~coroutine_ops() {
        if (handle_)
            handle_.destroy();
    }

    coroutine_ops(const coroutine_ops&) = delete;
    coroutine_ops& operator=(const coroutine_ops&) = delete;

    coroutine_ops(coroutine_ops&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    coroutine_ops& operator=(coroutine_ops&& other) noexcept {
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

// --- coroutine<Yield, Return> ---

template<typename Yield, typename Return>
struct coroutine;

// coroutine<Yield, Return> — value yields, value return

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
struct coroutine<Yield, Return> {
    struct promise_type {
        coroutine get_return_object() {
            return coroutine{
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

    explicit coroutine(std::coroutine_handle<promise_type> h) : ops_(h) {}

    coroutine(coroutine&&) = default;
    coroutine& operator=(coroutine&&) = default;
    coroutine(const coroutine&) = delete;
    coroutine& operator=(const coroutine&) = delete;

    void resume() { ops_.resume(); }
    bool done() const { return ops_.done(); }
    bool has_yield() const { return ops_.has_yield(); }

    Yield consume_yield() {
        if (!has_yield())
            throw std::logic_error("coroutine::consume_yield without yield");
        auto v = std::move(ops_.promise().yield_);
        ops_.promise().at_yield_ = false;
        return v;
    }

    Return result() const {
        if (!done() || !ops_.promise().return_ready_)
            throw std::logic_error("coroutine::result before completion");
        return ops_.promise().return_;
    }

private:
    coroutine_ops<promise_type> ops_;
};

// coroutine<Yield, void> — value yields, void return

template<typename Yield>
    requires(!std::is_void_v<Yield>)
struct coroutine<Yield, void> {
    struct promise_type {
        coroutine get_return_object() {
            return coroutine{
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

    explicit coroutine(std::coroutine_handle<promise_type> h) : ops_(h) {}

    coroutine(coroutine&&) = default;
    coroutine& operator=(coroutine&&) = default;
    coroutine(const coroutine&) = delete;
    coroutine& operator=(const coroutine&) = delete;

    void resume() { ops_.resume(); }
    bool done() const { return ops_.done(); }
    bool has_yield() const { return ops_.has_yield(); }

    Yield consume_yield() {
        if (!has_yield())
            throw std::logic_error("coroutine::consume_yield without yield");
        auto v = std::move(ops_.promise().yield_);
        ops_.promise().at_yield_ = false;
        return v;
    }

private:
    coroutine_ops<promise_type> ops_;
};

// coroutine<void, Return> — suspend with co_await std::suspend_always{}

template<typename Return>
    requires(!std::is_void_v<Return>)
struct coroutine<void, Return> {
    struct promise_type {
        coroutine get_return_object() {
            return coroutine{
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

    explicit coroutine(std::coroutine_handle<promise_type> h) : ops_(h) {}

    coroutine(coroutine&&) = default;
    coroutine& operator=(coroutine&&) = default;
    coroutine(const coroutine&) = delete;
    coroutine& operator=(const coroutine&) = delete;

    void resume() { ops_.resume(); }
    bool done() const { return ops_.done(); }

    Return result() const {
        if (!done() || !ops_.promise().return_ready_)
            throw std::logic_error("coroutine::result before completion");
        return ops_.promise().return_;
    }

private:
    coroutine_ops<promise_type> ops_;
};

// coroutine<void, void> — suspend with co_await std::suspend_always{}

template<>
struct coroutine<void, void> {
    struct promise_type {
        coroutine get_return_object() {
            return coroutine{
                std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        void unhandled_exception() { exception_ = std::current_exception(); }

        void return_void() { exception_ = nullptr; }

        bool at_yield_{false};
        std::exception_ptr exception_{};
    };

    explicit coroutine(std::coroutine_handle<promise_type> h) : ops_(h) {}

    coroutine(coroutine&&) = default;
    coroutine& operator=(coroutine&&) = default;
    coroutine(const coroutine&) = delete;
    coroutine& operator=(const coroutine&) = delete;

    void resume() { ops_.resume(); }
    bool done() const { return ops_.done(); }

private:
    coroutine_ops<promise_type> ops_;
};

#endif
