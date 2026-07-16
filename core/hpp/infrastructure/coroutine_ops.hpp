#ifndef COROUTINE_OPS_HPP
#define COROUTINE_OPS_HPP

#include <coroutine>
#include <exception>
#include <stdexcept>
#include <utility>

template<typename Promise>
struct coroutine_ops {
    coroutine_ops();
    coroutine_ops(std::coroutine_handle<Promise> h);

    ~coroutine_ops();

    coroutine_ops(const coroutine_ops&) = delete;
    coroutine_ops& operator=(const coroutine_ops&) = delete;

    coroutine_ops(coroutine_ops&& other) noexcept;
    coroutine_ops& operator=(coroutine_ops&& other) noexcept;

    void resume();
    bool done() const;
    bool has_yield() const;

    Promise& promise();
    const Promise& promise_const() const;

private:
    std::coroutine_handle<Promise> handle_;
};

template<typename Promise>
coroutine_ops<Promise>::coroutine_ops() : handle_() {}

template<typename Promise>
coroutine_ops<Promise>::coroutine_ops(std::coroutine_handle<Promise> h) : handle_(h) {}

template<typename Promise>
coroutine_ops<Promise>::~coroutine_ops() {
    if (handle_)
        handle_.destroy();
}

template<typename Promise>
coroutine_ops<Promise>::coroutine_ops(coroutine_ops&& other) noexcept : handle_(other.handle_) {
    other.handle_ = nullptr;
}

template<typename Promise>
coroutine_ops<Promise>& coroutine_ops<Promise>::operator=(coroutine_ops&& other) noexcept {
    if (this != &other) {
        if (handle_)
            handle_.destroy();
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }
    return *this;
}

template<typename Promise>
void coroutine_ops<Promise>::resume() {
    if (handle_.promise().exception_)
        std::rethrow_exception(handle_.promise().exception_);
    if (handle_.done())
        return;
    handle_.promise().at_yield_ = false;
    handle_.resume();
    if (handle_.promise().exception_)
        std::rethrow_exception(handle_.promise().exception_);
}

template<typename Promise>
bool coroutine_ops<Promise>::done() const { return !handle_ || handle_.done(); }

template<typename Promise>
bool coroutine_ops<Promise>::has_yield() const { return handle_ && handle_.promise().at_yield_; }

template<typename Promise>
Promise& coroutine_ops<Promise>::promise() { return handle_.promise(); }

template<typename Promise>
const Promise& coroutine_ops<Promise>::promise_const() const { return handle_.promise(); }

#endif
