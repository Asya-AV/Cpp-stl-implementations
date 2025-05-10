#include <iostream>

template <typename T>
class Deque {
private:
    static const size_t size_of_chunk = 32;

    template <bool is_const>
    class iterator_pattern {
    public:
        friend Deque;
        using iterator_category = std::random_access_iterator_tag;
        using value_type = std::conditional_t<is_const, const T, T>;
        using pointer = value_type*;
        using reference = value_type&;

    private:
        pointer const* chunk_in_map;
        pointer first_in_chunk;
        pointer cur_in_chunk;

        void update_iter(pointer const* new_chunk, ptrdiff_t dist = 0) {
            chunk_in_map = new_chunk;
            first_in_chunk = *new_chunk;
            cur_in_chunk = first_in_chunk + dist;
        }

    public:
        iterator_pattern& operator++() {
            *this += 1;
            return *this;
        }

        iterator_pattern& operator--() {
            *this -= 1;
            return *this;
        }

        iterator_pattern operator++(int) {
            iterator_pattern ret_iter = *this;
            ++(*this);
            return ret_iter;
        }

        iterator_pattern operator--(int) {
            iterator_pattern ret_iter = *this;
            --(*this);
            return ret_iter;
        }

        iterator_pattern& operator+=(ptrdiff_t dist) {
            ptrdiff_t new_dist = cur_in_chunk - first_in_chunk + dist;
            if (new_dist < ptrdiff_t(size_of_chunk) && new_dist >= 0) {
                cur_in_chunk += dist;
                return *this;
            }
            if (new_dist >= 0) {
                update_iter(chunk_in_map + new_dist / size_of_chunk,
                            new_dist % size_of_chunk);
                return *this;
            }
            ptrdiff_t cnt_chunk_dif =
                    -((-new_dist + size_of_chunk - 1) / size_of_chunk);
            update_iter(chunk_in_map + cnt_chunk_dif,
                        new_dist - cnt_chunk_dif * size_of_chunk);
            return *this;
        }

        iterator_pattern& operator-=(ptrdiff_t dist) {
            *this += -dist;
            return *this;
        }

        iterator_pattern operator+(ptrdiff_t dist) const {
            iterator_pattern ret_it = *this;
            ret_it += dist;
            return ret_it;
        }

        iterator_pattern operator-(ptrdiff_t dist) const {
            iterator_pattern ret_it = *this;
            ret_it -= dist;
            return ret_it;
        }

        ptrdiff_t operator-(const iterator_pattern& other) const {
            return (chunk_in_map - other.chunk_in_map) * size_of_chunk +
                   (cur_in_chunk - first_in_chunk) -
                   (other.cur_in_chunk - other.first_in_chunk);
        }

        bool operator==(const iterator_pattern& other) const {
            return cur_in_chunk == other.cur_in_chunk;
        }

        bool operator!=(const iterator_pattern& other) const {
            return !(*this == other);
        }

        bool operator<(const iterator_pattern& other) const {
            return (*this - other) < 0;
        }

        bool operator>(const iterator_pattern& other) const {
            return (*this - other) > 0;
        }

        bool operator<=(const iterator_pattern& other) const {
            return !(*this > other);
        }

        bool operator>=(const iterator_pattern& other) const {
            return !(*this < other);
        }

        reference operator*() const { return *cur_in_chunk; }

        pointer operator->() const { return cur_in_chunk; }

        operator iterator_pattern<true>() const {
            iterator_pattern<true> ret_it;
            ret_it.update_iter(chunk_in_map, cur_in_chunk - first_in_chunk);
            return ret_it;
        }
    };

public:
    using iterator = iterator_pattern<false>;
    using const_iterator = iterator_pattern<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    T** map_;
    size_t map_size_;
    iterator start_, finish_;

    T** construct_map(size_t sz) {
        map_size_ = sz;
        T** new_chunk = new T*[map_size_];
        for (size_t i = 0; i < map_size_; ++i) {
            new_chunk[i] =
                    reinterpret_cast<T*>(operator new(size_of_chunk * sizeof(T)));
        }
        return new_chunk;
    }

    void construct_iter(size_t sz) {
        start_.update_iter(map_ + 1);
        if (sz != 0 && sz % size_of_chunk == 0) {
            finish_.update_iter(map_ + map_size_ - 1);
        } else {
            finish_.update_iter(map_ + map_size_ - 2, sz % size_of_chunk);
        }
    }

    void resize_map() {
        T** large_map = new T*[3 * map_size_];
        for (size_t ind = 0; ind < 3 * map_size_; ++ind) {
            if (ind >= map_size_ && ind < 2 * map_size_) {
                large_map[ind] = map_[ind - map_size_];
            } else {
                large_map[ind] =
                        reinterpret_cast<T*>(new char[size_of_chunk * sizeof(T)]);
            }
        }

        ptrdiff_t old_start_map = start_.chunk_in_map - map_;
        ptrdiff_t old_start_chunk = start_.cur_in_chunk - start_.first_in_chunk;
        ptrdiff_t old_finish_map = finish_.chunk_in_map - map_;
        ptrdiff_t old_finish_chunk = finish_.cur_in_chunk - finish_.first_in_chunk;
        size_t old_map_size = map_size_;

        delete[] map_;
        map_ = large_map;
        map_size_ *= 3;
        start_.update_iter(map_ + old_map_size + old_start_map, old_start_chunk);
        finish_.update_iter(map_ + old_map_size + old_finish_map, old_finish_chunk);
    }

public:
    Deque() : map_(construct_map(3)) { construct_iter(0); }

    Deque(const Deque& other) : map_(construct_map(other.map_size_)) {
        construct_iter(other.size());
        const_iterator other_it = other.begin();
        for (auto it = begin(); it != end(); ++it, ++other_it) {
            try {
                new (it.cur_in_chunk) T(*(other_it));
            } catch (...) {
                for (auto del_it = begin(); del_it != it; ++del_it) {
                    (*del_it).~T();
                }
                throw;
            }
        }
    }

    Deque(int sz)
            : map_(construct_map((sz + size_of_chunk - 1) / size_of_chunk + 2)) {
        construct_iter(sz);
        for (auto it = begin(); it != end(); ++it) {
            try {
                new (it.cur_in_chunk) T();
            } catch (...) {
                for (auto del_it = begin(); del_it != it; ++del_it) {
                    (*del_it).~T();
                }
                throw;
            }
        }
    }

    Deque(int sz, const T& value)
            : map_(construct_map((sz + size_of_chunk - 1) / size_of_chunk + 2)) {
        construct_iter(sz);
        for (auto it = begin(); it != end(); ++it) {
            try {
                new (it.cur_in_chunk) T(value);
            } catch (...) {
                for (auto del_it = begin(); del_it != it; ++del_it) {
                    (*del_it).~T();
                }
                throw;
            }
        }
    }

    Deque& operator=(Deque other) {
        std::swap(other.map_, map_);
        std::swap(other.map_size_, map_size_);
        std::swap(other.start_, start_);
        std::swap(other.finish_, finish_);
        return *this;
    }

    size_t size() const { return end() - begin(); }

    iterator begin() { return start_; }

    const_iterator cbegin() const { return const_iterator(start_); }

    iterator end() { return finish_; }

    const_iterator cend() const { return const_iterator(finish_); }

    const_iterator begin() const { return cbegin(); }

    const_iterator end() const { return cend(); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }

    reverse_iterator rend() { return reverse_iterator(begin()); }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator(cbegin());
    }

    const_reverse_iterator rbegin() const { return crbegin(); }

    const_reverse_iterator rend() const { return crend(); }

    T& operator[](size_t index) { return *(begin() + index); }

    const T& operator[](size_t index) const { return *(begin() + index); }

    T& at(size_t index) {
        if (index < size()) {
            return (*this)[index];
        }
        throw std::out_of_range("cringe...");
    }

    const T& at(size_t index) const {
        if (index < size()) {
            return (*this)[index];
        }
        throw std::out_of_range("cringe...");
    }

    void pop_back() {
        --finish_;
        end()->~T();
    }

    void pop_front() {
        begin()->~T();
        ++start_;
    }

    void push_back(const T& value) {
        try {
            new (end().cur_in_chunk) T(value);
        } catch (...) {
            throw;
        }
        ++finish_;
        if (map_[map_size_ - 1] == (end() - 1).cur_in_chunk) {
            resize_map();
        }
    }

    void push_front(const T& value) {
        --start_;
        try {
            new (begin().cur_in_chunk) T(value);
        } catch (...) {
            throw;
        }
        if (map_[1] == (begin() + 1).cur_in_chunk) {
            resize_map();
        }
    }

    void insert(iterator place, const T& value) {
        if (end().cur_in_chunk == map_[map_size_ - 1]) {
            resize_map();
        }
        for (auto it = end(); it != place; --it) {
            new (it.cur_in_chunk) T(*(it - 1));
            (it - 1)->~T();
        }
        new (place.cur_in_chunk) T(value);
        ++finish_;
    }

    void erase(iterator pos) {
        for (auto it = pos; it != end(); ++it) {
            std::swap(*it, *(it + 1));
        }
        pop_back();
    }

    ~Deque() {
        for (auto it = begin(); it != end(); ++it) {
            it->~T();
        }
        for (size_t chunk_ind = 0; chunk_ind < map_size_; ++chunk_ind) {
            operator delete(map_[chunk_ind]);
        }
        delete[] map_;
    }
};
