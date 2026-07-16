#ifndef COROUTINE_HPP
#define COROUTINE_HPP

#include <coroutine>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "infrastructure/coroutine_ops.hpp"

template<typename Yield, typename Return>
struct coroutine;

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
struct coroutine<Yield, Return> {
    struct promise_type {
        promise_type();

        coroutine get_return_object();
        std::suspend_always initial_suspend();
        std::suspend_always final_suspend() noexcept;
        void unhandled_exception();
        std::suspend_always yield_value(Yield v);
        void return_value(Return r);

        Yield yield_;
        Return return_;
        bool return_ready_;
        bool at_yield_;
        std::exception_ptr exception_;
    };

    coroutine(std::coroutine_handle<promise_type> h);

    coroutine(coroutine&&) = default;
    coroutine& operator=(coroutine&&) = default;
    coroutine(const coroutine&) = delete;
    coroutine& operator=(const coroutine&) = delete;

    void resume();
    bool done() const;
    bool has_yield() const;
    Yield consume_yield();
    Return result() const;

private:
    coroutine_ops<promise_type> ops_;
};

template<typename Yield>
    requires(!std::is_void_v<Yield>)
struct coroutine<Yield, void> {
    struct promise_type {
        promise_type();

        coroutine get_return_object();
        std::suspend_always initial_suspend();
        std::suspend_always final_suspend() noexcept;
        void unhandled_exception();
        std::suspend_always yield_value(Yield v);
        void return_void();

        Yield yield_;
        bool at_yield_;
        std::exception_ptr exception_;
    };

    coroutine(std::coroutine_handle<promise_type> h);

    coroutine(coroutine&&) = default;
    coroutine& operator=(coroutine&&) = default;
    coroutine(const coroutine&) = delete;
    coroutine& operator=(const coroutine&) = delete;

    void resume();
    bool done() const;
    bool has_yield() const;
    Yield consume_yield();

private:
    coroutine_ops<promise_type> ops_;
};

template<typename Return>
    requires(!std::is_void_v<Return>)
struct coroutine<void, Return> {
    struct promise_type {
        promise_type();

        coroutine get_return_object();
        std::suspend_always initial_suspend();
        std::suspend_always final_suspend() noexcept;
        void unhandled_exception();
        void return_value(Return r);

        Return return_;
        bool return_ready_;
        bool at_yield_;
        std::exception_ptr exception_;
    };

    coroutine(std::coroutine_handle<promise_type> h);

    coroutine(coroutine&&) = default;
    coroutine& operator=(coroutine&&) = default;
    coroutine(const coroutine&) = delete;
    coroutine& operator=(const coroutine&) = delete;

    void resume();
    bool done() const;
    Return result() const;

private:
    coroutine_ops<promise_type> ops_;
};

template<>
struct coroutine<void, void> {
    struct promise_type {
        promise_type();

        coroutine get_return_object();
        std::suspend_always initial_suspend();
        std::suspend_always final_suspend() noexcept;
        void unhandled_exception();
        void return_void();

        bool at_yield_;
        std::exception_ptr exception_;
    };

    coroutine(std::coroutine_handle<promise_type> h);

    coroutine(coroutine&&) = default;
    coroutine& operator=(coroutine&&) = default;
    coroutine(const coroutine&) = delete;
    coroutine& operator=(const coroutine&) = delete;

    void resume();
    bool done() const;

private:
    coroutine_ops<promise_type> ops_;
};

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
coroutine<Yield, Return>::promise_type::promise_type()
    : return_ready_(false), at_yield_(false), exception_(nullptr) {}

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
coroutine<Yield, Return>
coroutine<Yield, Return>::promise_type::get_return_object() {
    return coroutine{std::coroutine_handle<promise_type>::from_promise(*this)};
}

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
std::suspend_always coroutine<Yield, Return>::promise_type::initial_suspend() { return {}; }

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
std::suspend_always coroutine<Yield, Return>::promise_type::final_suspend() noexcept { return {}; }

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
void coroutine<Yield, Return>::promise_type::unhandled_exception() {
    exception_ = std::current_exception();
}

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
std::suspend_always coroutine<Yield, Return>::promise_type::yield_value(Yield v) {
    yield_ = std::move(v);
    at_yield_ = true;
    return {};
}

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
void coroutine<Yield, Return>::promise_type::return_value(Return r) {
    return_ = std::move(r);
    return_ready_ = true;
    at_yield_ = false;
    exception_ = nullptr;
}

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
coroutine<Yield, Return>::coroutine(std::coroutine_handle<promise_type> h) : ops_(h) {}

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
void coroutine<Yield, Return>::resume() { ops_.resume(); }

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
bool coroutine<Yield, Return>::done() const { return ops_.done(); }

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
bool coroutine<Yield, Return>::has_yield() const { return ops_.has_yield(); }

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
Yield coroutine<Yield, Return>::consume_yield() {
    if (!has_yield())
        throw std::logic_error("coroutine::consume_yield without yield");
    auto v = std::move(ops_.promise().yield_);
    ops_.promise().at_yield_ = false;
    return v;
}

template<typename Yield, typename Return>
    requires(!std::is_void_v<Yield> && !std::is_void_v<Return>)
Return coroutine<Yield, Return>::result() const {
    if (!done() || !ops_.promise_const().return_ready_)
        throw std::logic_error("coroutine::result before completion");
    return ops_.promise_const().return_;
}

template<typename Yield>
    requires(!std::is_void_v<Yield>)
coroutine<Yield, void>::promise_type::promise_type()
    : at_yield_(false), exception_(nullptr) {}

template<typename Yield>
    requires(!std::is_void_v<Yield>)
coroutine<Yield, void>
coroutine<Yield, void>::promise_type::get_return_object() {
    return coroutine{std::coroutine_handle<promise_type>::from_promise(*this)};
}

template<typename Yield>
    requires(!std::is_void_v<Yield>)
std::suspend_always coroutine<Yield, void>::promise_type::initial_suspend() { return {}; }

template<typename Yield>
    requires(!std::is_void_v<Yield>)
std::suspend_always coroutine<Yield, void>::promise_type::final_suspend() noexcept { return {}; }

template<typename Yield>
    requires(!std::is_void_v<Yield>)
void coroutine<Yield, void>::promise_type::unhandled_exception() {
    exception_ = std::current_exception();
}

template<typename Yield>
    requires(!std::is_void_v<Yield>)
std::suspend_always coroutine<Yield, void>::promise_type::yield_value(Yield v) {
    yield_ = std::move(v);
    at_yield_ = true;
    return {};
}

template<typename Yield>
    requires(!std::is_void_v<Yield>)
void coroutine<Yield, void>::promise_type::return_void() {
    at_yield_ = false;
    exception_ = nullptr;
}

template<typename Yield>
    requires(!std::is_void_v<Yield>)
coroutine<Yield, void>::coroutine(std::coroutine_handle<promise_type> h) : ops_(h) {}

template<typename Yield>
    requires(!std::is_void_v<Yield>)
void coroutine<Yield, void>::resume() { ops_.resume(); }

template<typename Yield>
    requires(!std::is_void_v<Yield>)
bool coroutine<Yield, void>::done() const { return ops_.done(); }

template<typename Yield>
    requires(!std::is_void_v<Yield>)
bool coroutine<Yield, void>::has_yield() const { return ops_.has_yield(); }

template<typename Yield>
    requires(!std::is_void_v<Yield>)
Yield coroutine<Yield, void>::consume_yield() {
    if (!has_yield())
        throw std::logic_error("coroutine::consume_yield without yield");
    auto v = std::move(ops_.promise().yield_);
    ops_.promise().at_yield_ = false;
    return v;
}

template<typename Return>
    requires(!std::is_void_v<Return>)
coroutine<void, Return>::promise_type::promise_type()
    : return_ready_(false), at_yield_(false), exception_(nullptr) {}

template<typename Return>
    requires(!std::is_void_v<Return>)
coroutine<void, Return>
coroutine<void, Return>::promise_type::get_return_object() {
    return coroutine{std::coroutine_handle<promise_type>::from_promise(*this)};
}

template<typename Return>
    requires(!std::is_void_v<Return>)
std::suspend_always coroutine<void, Return>::promise_type::initial_suspend() { return {}; }

template<typename Return>
    requires(!std::is_void_v<Return>)
std::suspend_always coroutine<void, Return>::promise_type::final_suspend() noexcept { return {}; }

template<typename Return>
    requires(!std::is_void_v<Return>)
void coroutine<void, Return>::promise_type::unhandled_exception() {
    exception_ = std::current_exception();
}

template<typename Return>
    requires(!std::is_void_v<Return>)
void coroutine<void, Return>::promise_type::return_value(Return r) {
    return_ = std::move(r);
    return_ready_ = true;
    exception_ = nullptr;
}

template<typename Return>
    requires(!std::is_void_v<Return>)
coroutine<void, Return>::coroutine(std::coroutine_handle<promise_type> h) : ops_(h) {}

template<typename Return>
    requires(!std::is_void_v<Return>)
void coroutine<void, Return>::resume() { ops_.resume(); }

template<typename Return>
    requires(!std::is_void_v<Return>)
bool coroutine<void, Return>::done() const { return ops_.done(); }

template<typename Return>
    requires(!std::is_void_v<Return>)
Return coroutine<void, Return>::result() const {
    if (!done() || !ops_.promise_const().return_ready_)
        throw std::logic_error("coroutine::result before completion");
    return ops_.promise_const().return_;
}

inline coroutine<void, void>::promise_type::promise_type()
    : at_yield_(false), exception_(nullptr) {}

inline coroutine<void, void>
coroutine<void, void>::promise_type::get_return_object() {
    return coroutine{std::coroutine_handle<promise_type>::from_promise(*this)};
}

inline std::suspend_always coroutine<void, void>::promise_type::initial_suspend() { return {}; }

inline std::suspend_always coroutine<void, void>::promise_type::final_suspend() noexcept { return {}; }

inline void coroutine<void, void>::promise_type::unhandled_exception() {
    exception_ = std::current_exception();
}

inline void coroutine<void, void>::promise_type::return_void() { exception_ = nullptr; }

inline coroutine<void, void>::coroutine(std::coroutine_handle<promise_type> h) : ops_(h) {}

inline void coroutine<void, void>::resume() { ops_.resume(); }

inline bool coroutine<void, void>::done() const { return ops_.done(); }

#endif
