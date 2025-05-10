#include <iostream>
#include <list>
#include <memory>

template <size_t N>
struct StackStorage {
    StackStorage() = default;

    char data[N];
    char* storage_top{data};
};

template <typename T, size_t N>
class StackAllocator {
public:
    StackStorage<N>* storage;

    using value_type = T;

    StackAllocator() = default;

    StackAllocator(StackStorage<N>& storage) : storage(&storage) {}

    template <typename U>
    StackAllocator(const StackAllocator<U, N>& other) : storage(other.storage) {}

    template <typename U>
    struct rebind {
        using other = StackAllocator<U, N>;
    };

    bool operator==(const StackAllocator& other) const {
        return storage == other.storage;
    }

    T* allocate(size_t count) {
        void* oldtop = storage->storage_top;
        std::size_t sz = N - (storage->storage_top - storage->data);
        std::align(alignof(T), count * sizeof(T), oldtop, sz);
        storage->storage_top += count * sizeof(T);
        return reinterpret_cast<T*>(oldtop);
    }

    void deallocate(T*, size_t) {}
};

template <typename T, typename Allocator = std::allocator<T>>
class List {
    struct BaseNode {
        BaseNode* prev;
        BaseNode* next;

        BaseNode() : prev(this), next(this) {}
    };

    struct Node : public BaseNode {
        T data;

        Node() = default;

        Node(const T& data) : data(data) {}
    };

    using NodeAllocator =
            typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using allocator_traits = std::allocator_traits<NodeAllocator>;

    BaseNode fake_node_;
    size_t size_;
    [[no_unique_address]] NodeAllocator allocator_;

    template <bool is_const>
    struct base_list_iterator {
        BaseNode* node;

        using value_type = typename std::conditional_t<is_const, const T, T>;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;
        using pointer = value_type*;
        using reference = value_type&;

        base_list_iterator() = default;

        base_list_iterator(BaseNode* node) : node(node) {}

        base_list_iterator(const BaseNode* node)
                : node(const_cast<BaseNode*>(node)) {}

        operator base_list_iterator<true>() const {
            base_list_iterator<true> cur(node);
            return cur;
        }

        base_list_iterator& operator++() {
            node = node->next;
            return *this;
        }

        base_list_iterator operator++(int) {
            base_list_iterator utility = node;
            node = node->next;
            return utility;
        }

        base_list_iterator& operator--() {
            node = node->prev;
            return *this;
        }

        base_list_iterator operator--(int) {
            base_list_iterator utility = node;
            node = node->prev;
            return utility;
        }

        reference operator*() const { return static_cast<Node*>(node)->data; }

        pointer operator->() const { return &static_cast<Node*>(node)->data; }

        bool operator==(const base_list_iterator& other) const {
            return node == other.node;
        }

        bool operator!=(const base_list_iterator& other) const {
            return node != other.node;
        }
    };

    void clear(size_t ind) {
        for (size_t i = 0; i < ind; ++i) {
            pop_back();
        }
    }

    template <typename... Args>
    void fill_list(size_t count, const Args&... args) {
        size_t ind = 0;
        try {
            for (size_t i = 0; i < count; ++i, ++ind) {
                base_insert(end(), args...);
            }
        } catch (...) {
            clear(ind);
            throw;
        }
    }

public:
    using iterator = base_list_iterator<false>;
    using const_iterator = base_list_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    Allocator get_allocator() const { return allocator_; }

    List() : fake_node_(), size_(0), allocator_() {}

    List(size_t count) : fake_node_(), size_(0), allocator_() {
        fill_list(count);
    }

    List(size_t count, const T& data) : fake_node_(), size_(0), allocator_() {
        fill_list(count, data);
    }

    List(const Allocator& allocator)
            : fake_node_(), size_(0), allocator_(allocator) {}

    List(size_t count, const Allocator& allocator)
            : fake_node_(), size_(0), allocator_(allocator) {
        fill_list(count);
    }

    List(size_t count, const T& data, const Allocator& allocator)
            : fake_node_(), size_(0), allocator_(allocator) {
        fill_list(count, data);
    }


    List(const List& other)
            : fake_node_(),
              size_(0),
              allocator_(
                      std::allocator_traits<NodeAllocator>::
                      select_on_container_copy_construction(other.allocator_)) {
        size_t ind = 0;
        try {
            for (auto& now : other) {
                push_back(now);
                ++ind;
            }
        } catch (...) {
            clear(ind);
            throw;
        }
    }

    List& operator=(const List& other) {
        List cur = other;
        if (!std::allocator_traits<
                NodeAllocator>::propagate_on_container_swap::value) {
            swap(cur);
        }

        allocator_ =
                std::allocator_traits<
                        NodeAllocator>::propagate_on_container_copy_assignment::value
                ? other.allocator_
                : allocator_;
        return *this;
    }

    void swap(List& other) {
        std::swap(other.fake_node_, fake_node_);
        std::swap(other.fake_node_.prev->next, fake_node_.prev->next);
        std::swap(other.fake_node_.next->prev, fake_node_.next->prev);
        std::swap(other.size_, size_);
        std::swap(other.allocator_, allocator_);
    }

    ~List() {
        while (size_ != 0) {
            pop_back();
        }
    }

    size_t size() const { return size_; }

    template <typename... Args>
    void base_insert(const_iterator iter, Args&... args) {
        Node* new_node;
        try {
            ++size_;
            new_node = allocator_traits::allocate(allocator_, 1);
            allocator_traits::construct(allocator_, new_node, args...);
            new_node->prev = iter.node->prev;
            new_node->next = iter.node;
            iter.node->prev->next = new_node;
            iter.node->prev = new_node;
        } catch (...) {
            allocator_traits::deallocate(allocator_, new_node, 1);
            throw;
        }
    }

    void insert(const_iterator iter, const T& data) { base_insert(iter, data); }

    void erase(const_iterator iter) {
        --size_;
        iter.node->prev->next = iter.node->next;
        iter.node->next->prev = iter.node->prev;
        allocator_traits::destroy(allocator_, static_cast<Node*>(iter.node));
        allocator_traits::deallocate(allocator_, static_cast<Node*>(iter.node), 1);
    }

    iterator begin() { return fake_node_.next; }

    const_iterator begin() const { return cbegin(); }

    iterator end() { return &fake_node_; }

    const_iterator end() const { return cend(); }

    const_iterator cbegin() const { return fake_node_.next; }

    const_iterator cend() const { return &fake_node_; }

    reverse_iterator rbegin() { return reverse_iterator(end()); }

    const_reverse_iterator rbegin() const { return crbegin(); }

    reverse_iterator rend() { return reverse_iterator(begin()); }

    const_reverse_iterator rend() const { return crend(); }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator(begin());
    }

    void push_back(const T& data) { insert(end(), data); }

    void push_front(const T& data) { insert(begin(), data); }

    void pop_back() {
        iterator iter = end();
        --iter;
        erase(iter);
    }

    void pop_front() {
        iterator iter = begin();
        erase(iter);
    }
};
