#include <memory>
#include <type_traits>
#include <utility>

struct ControlBlock {
    size_t shared_count = 1;
    size_t weak_count = 0;
    virtual void deallocate() = 0;
    virtual void getDeleter() = 0;
    virtual ~ControlBlock(){};
};

template <typename T, typename Deleter, typename Allocator>
struct BaseControlBlock : ControlBlock {
    T* object;
    Deleter deleter;
    Allocator alloc;

    explicit BaseControlBlock(T* object) : object(object) {}
    BaseControlBlock(T* obj, Deleter deleter) : object(obj), deleter(deleter) {}
    BaseControlBlock(T* obj, Deleter deleter, Allocator alloc)
            : object(obj), deleter(deleter), alloc(alloc) {}

    virtual void deallocate() {
        using CurAllocator =
                typename std::allocator_traits<Allocator>::template rebind_alloc<
                        BaseControlBlock<T, Deleter, Allocator>>;
        CurAllocator newAlloc = alloc;
        std::allocator_traits<CurAllocator>::deallocate(newAlloc, this, 1);
    }
    virtual void getDeleter() { deleter(object); }
};

template <typename T, typename Allocator>
struct SharedControlBlock : ControlBlock {
    T object;
    Allocator alloc;

    template <typename... Args>
    explicit SharedControlBlock(const Allocator& alloc, Args&&... args)
            : object(std::forward<Args>(args)...), alloc(alloc) {}

    virtual void deallocate() {
        using CurAllocator = typename std::allocator_traits<
                Allocator>::template rebind_alloc<SharedControlBlock<T, Allocator>>;
        CurAllocator newAlloc = alloc;
        std::allocator_traits<CurAllocator>::deallocate(newAlloc, this, 1);
    }
    virtual void getDeleter() {
        std::allocator_traits<Allocator>::destroy(alloc, &object);
    }
};

template <typename T>
class EnableSharedFromThis;

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr {
    T* ptr;
    ControlBlock* pcb;

    template <typename U>
    friend class WeakPtr;
    template <typename U>
    friend class SharedPtr;
    template <typename U, typename... Args>
    friend SharedPtr<U> makeShared(Args&&...);
    template <typename U, typename Allocator, typename... Args>
    friend SharedPtr<U> allocateShared(const Allocator&, Args&&...);

    template <typename U>
    void assertion() {
        static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U>);
    }

public:
    SharedPtr() : ptr(nullptr), pcb(nullptr) {}

    template <typename Allocator>
    SharedPtr(SharedControlBlock<T, Allocator>* cb) : ptr(&cb->object), pcb(cb) {}

    template <typename U>
    SharedPtr(U* cur_ptr)
            : ptr(cur_ptr),
              pcb(new BaseControlBlock<T, std::default_delete<T>, std::allocator<T>>(
                      ptr)) {
        assertion<U>();
        if constexpr (std::is_base_of_v<EnableSharedFromThis<U>, U>) {
            ptr->weak_ptr = *this;
        }
    }

    template <typename U, typename Deleter,
            typename Allocator = std::allocator<T>>
    SharedPtr(U* cur_ptr, Deleter deleter, Allocator alloc = std::allocator<T>())
            : ptr(cur_ptr) {
        assertion<U>();
        if constexpr (std::is_base_of_v<EnableSharedFromThis<U>, U>) {
            ptr->weak_ptr = *this;
        }

        using CurAllocator =
                typename std::allocator_traits<Allocator>::template rebind_alloc<
                        BaseControlBlock<T, Deleter, Allocator>>;
        CurAllocator cur_alloc = alloc;
        auto adress = std::allocator_traits<CurAllocator>::allocate(cur_alloc, 1);
        pcb = adress;
        new (adress) BaseControlBlock<T, Deleter, Allocator>(ptr, deleter, alloc);
    }

    SharedPtr(const SharedPtr& other) noexcept : ptr(other.ptr), pcb(other.pcb) {
        if (pcb != nullptr) {
            ++pcb->shared_count;
        }
    }

    template <typename U>
    SharedPtr(const SharedPtr<U>& other) : ptr(other.ptr), pcb(other.pcb) {
        assertion<U>();
        if (pcb != nullptr) {
            ++pcb->shared_count;
        }
    }

    SharedPtr(SharedPtr&& other) noexcept : ptr(other.ptr), pcb(other.pcb) {
        other.ptr = nullptr;
        other.pcb = nullptr;
    }

    template <typename U>
    SharedPtr(SharedPtr<U>&& other) : ptr(other.ptr), pcb(other.pcb) {
        assertion<U>();
        other.ptr = nullptr;
        other.pcb = nullptr;
    }

    template <typename U>
    explicit SharedPtr(const WeakPtr<U>& other) : ptr(other.ptr), pcb(other.pcb) {
        assertion<U>();
        if (pcb != nullptr) {
            ++pcb->shared_count;
        }
    }

    void swap(SharedPtr& other) noexcept {
        std::swap(ptr, other.ptr);
        std::swap(pcb, other.pcb);
    }

    T& operator*() const { return *ptr; }

    T* operator->() const { return ptr; }

    T* get() const { return ptr; }

    SharedPtr& operator=(const SharedPtr& other) noexcept {
        SharedPtr(other).swap(*this);
        return *this;
    }

    template <typename U>
    SharedPtr& operator=(const SharedPtr<U>& other) {
        assertion<U>();
        SharedPtr(other).swap(*this);
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        SharedPtr(std::move(other)).swap(*this);
        return *this;
    }

    template <typename U>
    SharedPtr& operator=(SharedPtr<U>&& other) {
        assertion<U>();
        SharedPtr(std::move(other)).swap(*this);
        return *this;
    }

    size_t use_count() const {
        if (pcb) {
            return pcb->shared_count;
        }
        return 0;
    }

    void reset() { SharedPtr<T>().swap(*this); }

    void reset(T* cur_ptr) { SharedPtr<T>(cur_ptr).swap(*this); }

    ~SharedPtr() {
        if (pcb != nullptr) {
            pcb->shared_count--;

            if (pcb->shared_count == 0) {
                pcb->getDeleter();

                if (pcb->weak_count == 0) {
                    pcb->deallocate();
                }
            }
        }
    }
};

template <typename T>
class WeakPtr {
    T* ptr;
    ControlBlock* pcb;

    template <typename U>
    friend class SharedPtr;
    template <typename U>
    friend class WeakPtr;

    template <typename U>
    void assertion() {
        static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U>);
    }

public:
    WeakPtr() : ptr(nullptr), pcb(nullptr) {}

    template <typename U>
    WeakPtr(const WeakPtr<U>& other) : ptr(other.ptr), pcb(other.pcb) {
        assertion<U>();
        if (pcb != nullptr) {
            ++pcb->weak_count;
        }
    }

    WeakPtr(const WeakPtr& other) : ptr(other.ptr), pcb(other.pcb) {
        if (pcb != nullptr) {
            ++pcb->weak_count;
        }
    }


    template <typename U>
    WeakPtr(const SharedPtr<U>& other) : ptr(other.ptr), pcb(other.pcb) {
        assertion<U>();
        if (pcb != nullptr) {
            ++pcb->weak_count;
        }
    }

    void swap(WeakPtr& other) noexcept {
        std::swap(ptr, other.ptr);
        std::swap(pcb, other.pcb);
    }

    WeakPtr& operator=(const SharedPtr<T>& other) noexcept {
        WeakPtr(other).swap(*this);
        return *this;
    }

    template <typename U>
    WeakPtr& operator=(const WeakPtr<U>& other) {
        assertion<U>();
        SharedPtr(other).swap(*this);
        return *this;
    }

    T* get() const { return ptr; }

    size_t use_count() const {
        return pcb == nullptr ? 0 : pcb->shared_count;
    }

    bool expired() const {
        return use_count() <= 0;
    }

    SharedPtr<T> lock() const {
        return expired() ? SharedPtr<T>() : SharedPtr<T>(*this);
    }

    ~WeakPtr() {
        if (pcb != nullptr) {
            --pcb->weak_count;

            if (pcb->shared_count == 0 && pcb->weak_count == 0) {
                pcb->deallocate();
            }
        }
    }
};

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
    auto p = new SharedControlBlock<T, std::allocator<T>>(
            std::allocator<T>(), std::forward<Args>(args)...);
    return SharedPtr<T>(p);
}

template <typename T, typename Allocator, typename... Args>
SharedPtr<T> allocateShared(const Allocator& alloc, Args&&... args) {
    using CurAllocator = typename std::allocator_traits<
            Allocator>::template rebind_alloc<SharedControlBlock<T, Allocator>>;
    CurAllocator newAlloc = alloc;
    auto adres = std::allocator_traits<CurAllocator>::allocate(newAlloc, 1);
    std::allocator_traits<CurAllocator>::construct(newAlloc, adres, alloc,
                                                   std::forward<Args>(args)...);
    return SharedPtr<T>(adres);
}

template <typename T>
class EnableSharedFromThis {
    WeakPtr<T> weak_ptr;

    template <typename U>
    friend class SharedPtr;

public:
    SharedPtr<T> shared_from_this() const { return weak_ptr.lock(); }
};
