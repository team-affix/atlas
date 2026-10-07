#ifndef INCREMENTAL_SET_HPP
#define INCREMENTAL_SET_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

template<typename Key>
struct incremental_set {
    struct node {
        Key key;
        uint32_t priority;
        std::shared_ptr<const node> left;
        std::shared_ptr<const node> right;
    };
    struct const_iterator {
        using difference_type   = std::ptrdiff_t;
        using value_type        = Key;
        using pointer           = const Key*;
        using reference         = const Key&;
        using iterator_category = std::forward_iterator_tag;

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
    using node_ptr = std::shared_ptr<const node>;

    static uint32_t priority_of(const Key& key);
    static node_ptr treap_insert(node_ptr n, const Key& key, uint32_t pri);
    static node_ptr treap_erase(node_ptr n, const Key& key);
    static node_ptr treap_merge(node_ptr left, node_ptr right);
    static node_ptr copy_with(const node* n, node_ptr left, node_ptr right);
    incremental_set(node_ptr root);

    node_ptr root_;
};

template<typename Key>
incremental_set<Key>::incremental_set()
    : root_(nullptr) {}

template<typename Key>
incremental_set<Key>::incremental_set(node_ptr root)
    : root_(std::move(root)) {}

template<typename Key>
uint32_t incremental_set<Key>::priority_of(const Key& key) {
    return static_cast<uint32_t>(std::hash<Key>{}(key) * 2654435761u);
}

template<typename Key>
typename incremental_set<Key>::node_ptr
incremental_set<Key>::copy_with(const node* n, node_ptr left, node_ptr right) {
    return std::make_shared<node>(n->key, n->priority, std::move(left), std::move(right));
}

template<typename Key>
typename incremental_set<Key>::node_ptr
incremental_set<Key>::treap_merge(node_ptr left, node_ptr right) {
    if (!left)  return right;
    if (!right) return left;
    if (left->priority >= right->priority) {
        node_ptr merged = treap_merge(left->right, right);
        return copy_with(left.get(), left->left, std::move(merged));
    }
    node_ptr merged = treap_merge(left, right->left);
    return copy_with(right.get(), std::move(merged), right->right);
}

template<typename Key>
typename incremental_set<Key>::node_ptr
incremental_set<Key>::treap_insert(node_ptr n, const Key& key, uint32_t pri) {
    if (!n)
        return std::make_shared<node>(key, pri, nullptr, nullptr);
    if (key == n->key)
        return n;
    if (key < n->key) {
        node_ptr new_left = treap_insert(n->left, key, pri);
        node_ptr updated  = copy_with(n.get(), new_left, n->right);
        if (new_left->priority <= updated->priority)
            return updated;
        return copy_with(new_left.get(), new_left->left,
                         copy_with(updated.get(), new_left->right, updated->right));
    }
    node_ptr new_right = treap_insert(n->right, key, pri);
    node_ptr updated   = copy_with(n.get(), n->left, new_right);
    if (new_right->priority <= updated->priority)
        return updated;
    return copy_with(new_right.get(),
                     copy_with(updated.get(), updated->left, new_right->left),
                     new_right->right);
}

template<typename Key>
typename incremental_set<Key>::node_ptr
incremental_set<Key>::treap_erase(node_ptr n, const Key& key) {
    if (!n)
        return nullptr;
    if (key < n->key)
        return copy_with(n.get(), treap_erase(n->left, key), n->right);
    if (n->key < key)
        return copy_with(n.get(), n->left, treap_erase(n->right, key));
    return treap_merge(n->left, n->right);
}

template<typename Key>
incremental_set<Key> incremental_set<Key>::insert(Key key) const {
    return incremental_set<Key>(treap_insert(root_, key, priority_of(key)));
}

template<typename Key>
incremental_set<Key> incremental_set<Key>::erase(Key key) const {
    if (!root_) return *this;
    return incremental_set<Key>(treap_erase(root_, key));
}

template<typename Key>
bool incremental_set<Key>::contains(Key key) const {
    const node* n = root_.get();
    while (n) {
        if (key == n->key) return true;
        if (key < n->key)  n = n->left.get();
        else               n = n->right.get();
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
        n = n->left.get();
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
    push_left(current->right.get());
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
    return const_iterator(root_.get());
}

template<typename Key>
typename incremental_set<Key>::const_iterator incremental_set<Key>::end() const {
    return const_iterator(nullptr);
}

#endif
