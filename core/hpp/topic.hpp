#ifndef TOPIC_HPP
#define TOPIC_HPP

#include <queue>
#include <unordered_map>

template <typename T>
struct topic {
    struct subscription {
        ~subscription();
        subscription(topic<T>&);
        bool empty() const;
        T consume();
        void purge();
    #ifndef DEBUG
    private:
    #endif
        topic<T>* parent;
        size_t id;
    };
    void produce(const T&);
#ifndef DEBUG
private:
#endif
    std::unordered_map<size_t, std::queue<T>> mailboxes;
    size_t next_id;
    friend subscription;
};

template <typename T>
topic<T>::subscription::~subscription() {
    parent->mailboxes.erase(id);
}

template <typename T>
bool topic<T>::subscription::empty() const {
    return parent->mailboxes.at(id).empty();
}

template <typename T>
T topic<T>::subscription::consume() {
    auto& mailbox = parent->mailboxes.at(id);
    T result = mailbox.front();
    mailbox.pop();
    return result;
}

template <typename T>
void topic<T>::subscription::purge() {
    parent->mailboxes.at(id) = {};
}

template <typename T>
topic<T>::subscription::subscription(topic<T>& parent) : parent(&parent), id(parent.next_id++) {    
    parent.mailboxes.insert({id, std::queue<T>()});
}

template <typename T>
void topic<T>::produce(const T& item) {
    for (auto& [_, mailbox] : mailboxes) {
        mailbox.push(item);
    }
}

#endif
