#ifndef INCREMENTAL_SET_HPP
#define INCREMENTAL_SET_HPP

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <vector>

template<typename Key>
struct incremental_set {
    struct node {
        Key key;
        uint32_t priority;
        const node* left;
        const node* right;
    };
    struct const_iterator {
        const Key& operator*() const;
        const_iterator& operator++();
        bool operator==(const const_iterator& other) const;
        bool operator!=(const const_iterator& other) const;
        const_iterator(const node* root);
    private:
        void push_left(const node* n);

        std::vector<const node*> stack_;
    };

    incremental_set();
    incremental_set insert(Key key) const;
    incremental_set erase(Key key) const;
    bool contains(Key key) const;
    const_iterator begin() const;
    const_iterator end() const;
    
private:
    using pool_t = std::deque<node>;

    static uint32_t priority_of(const Key& key);
    static const node* treap_insert(const node* n, const Key& key, uint32_t pri, std::shared_ptr<pool_t>& pool);
    static const node* treap_erase(const node* n, const Key& key, std::shared_ptr<pool_t>& pool);
    static const node* treap_merge(const node* left, const node* right, std::shared_ptr<pool_t>& pool);
    static const node* copy_with(const node* n, const node* left, const node* right, std::shared_ptr<pool_t>& pool);
    explicit incremental_set(std::shared_ptr<pool_t> pool, const node* root);

    std::shared_ptr<pool_t> pool_;
    const node* root_;
};

template<typename Key>
incremental_set<Key>::incremental_set()
    : pool_(nullptr), root_(nullptr) {}

template<typename Key>
incremental_set<Key>::incremental_set(std::shared_ptr<pool_t> pool, const node* root)
    : pool_(std::move(pool)), root_(root) {}

template<typename Key>
uint32_t incremental_set<Key>::priority_of(const Key& key) {
    return static_cast<uint32_t>(std::hash<Key>{}(key) * 2654435761u);
}

template<typename Key>
const typename incremental_set<Key>::node*
incremental_set<Key>::copy_with(const node* n, const node* left, const node* right,
                                std::shared_ptr<pool_t>& pool) {
    pool->push_back(node{n->key, n->priority, left, right});
    return &pool->back();
}

template<typename Key>
const typename incremental_set<Key>::node*
incremental_set<Key>::treap_merge(const node* left, const node* right,
                                  std::shared_ptr<pool_t>& pool) {
    if (!left)  return right;
    if (!right) return left;
    if (left->priority >= right->priority) {
        const node* merged = treap_merge(left->right, right, pool);
        return copy_with(left, left->left, merged, pool);
    }
    const node* merged = treap_merge(left, right->left, pool);
    return copy_with(right, merged, right->right, pool);
}

template<typename Key>
const typename incremental_set<Key>::node*
incremental_set<Key>::treap_insert(const node* n, const Key& key, uint32_t pri,
                                   std::shared_ptr<pool_t>& pool) {
    if (!n) {
        pool->push_back(node{key, pri, nullptr, nullptr});
        return &pool->back();
    }
    if (key == n->key)
        return n;
    if (key < n->key) {
        const node* new_left = treap_insert(n->left, key, pri, pool);
        const node* updated  = copy_with(n, new_left, n->right, pool);
        if (new_left->priority <= updated->priority)
            return updated;
        return copy_with(new_left, new_left->left,
                         copy_with(updated, new_left->right, updated->right, pool),
                         pool);
    }
    const node* new_right = treap_insert(n->right, key, pri, pool);
    const node* updated   = copy_with(n, n->left, new_right, pool);
    if (new_right->priority <= updated->priority)
        return updated;
    return copy_with(new_right,
                     copy_with(updated, updated->left, new_right->left, pool),
                     new_right->right,
                     pool);
}

template<typename Key>
const typename incremental_set<Key>::node*
incremental_set<Key>::treap_erase(const node* n, const Key& key,
                                  std::shared_ptr<pool_t>& pool) {
    if (!n)
        return nullptr;
    if (key < n->key)
        return copy_with(n, treap_erase(n->left, key, pool), n->right, pool);
    if (n->key < key)
        return copy_with(n, n->left, treap_erase(n->right, key, pool), pool);
    return treap_merge(n->left, n->right, pool);
}

template<typename Key>
incremental_set<Key> incremental_set<Key>::insert(Key key) const {
    auto pool = pool_ ? pool_ : std::make_shared<pool_t>();
    const uint32_t pri = priority_of(key);
    const node* new_root = treap_insert(root_, key, pri, pool);
    return incremental_set<Key>(std::move(pool), new_root);
}

template<typename Key>
incremental_set<Key> incremental_set<Key>::erase(Key key) const {
    if (!root_)
        return *this;
    auto pool = pool_;
    const node* new_root = treap_erase(root_, key, pool);
    return incremental_set<Key>(std::move(pool), new_root);
}

template<typename Key>
bool incremental_set<Key>::contains(Key key) const {
    const node* n = root_;
    while (n) {
        if (key == n->key) return true;
        if (key < n->key)  n = n->left;
        else               n = n->right;
    }
    return false;
}

// const_iterator

template<typename Key>
incremental_set<Key>::const_iterator::const_iterator(const node* root) {
    push_left(root);
}

template<typename Key>
void incremental_set<Key>::const_iterator::push_left(const node* n) {
    while (n) {
        stack_.push_back(n);
        n = n->left;
    }
}

template<typename Key>
const Key& incremental_set<Key>::const_iterator::operator*() const {
    return stack_.back()->key;
}

template<typename Key>
typename incremental_set<Key>::const_iterator&
incremental_set<Key>::const_iterator::operator++() {
    const node* current = stack_.back();
    stack_.pop_back();
    push_left(current->right);
    return *this;
}

template<typename Key>
bool incremental_set<Key>::const_iterator::operator==(const const_iterator& other) const {
    return stack_ == other.stack_;
}

template<typename Key>
bool incremental_set<Key>::const_iterator::operator!=(const const_iterator& other) const {
    return stack_ != other.stack_;
}

template<typename Key>
typename incremental_set<Key>::const_iterator incremental_set<Key>::begin() const {
    return const_iterator(root_);
}

template<typename Key>
typename incremental_set<Key>::const_iterator incremental_set<Key>::end() const {
    return const_iterator(nullptr);
}

#endif
