#pragma once

#include <atomic>
#include <cstdio>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include "smart-ptr-base.h"

namespace my {

struct shared_ptr_control_block {
    virtual void delete_controlled_object() noexcept = 0;
    virtual ~shared_ptr_control_block() = default;

    std::atomic<int> m_use_count {1};
    std::atomic<int> m_weak_count {1};
};

template<class DeletionLambda>
struct shared_ptr_control_block_impl : public shared_ptr_control_block {
    shared_ptr_control_block_impl(DeletionLambda&& d) : m_deleter(std::move(d)) {}
    void delete_controlled_object() noexcept override { m_deleter(); }

    DeletionLambda m_deleter;
};

template<typename T>
class shared_ptr : public detail::smart_ptr_base<T>
{
    template<class Y> friend class shared_ptr;

    using detail::smart_ptr_base<T>::m_ptr;
public:
    using element_type = typename detail::smart_ptr_base<T>::element_type;

    constexpr shared_ptr() noexcept = default;
    constexpr shared_ptr(decltype(nullptr)) noexcept {}

    explicit shared_ptr(element_type *ptr) : shared_ptr(ptr, std::default_delete<T>{}) {}
    template<class Deleter> explicit shared_ptr(decltype(nullptr), Deleter d) : shared_ptr((T*)nullptr, std::move(d)) {}

    template<class Y, class = std::enable_if_t<!std::is_array_v<T>>>
    explicit shared_ptr(Y *ptr) : shared_ptr(ptr, std::default_delete<Y>{}) {}

    template<class Y, class Deleter> explicit shared_ptr(Y *ptr, Deleter d) {
        static_assert(std::is_nothrow_move_constructible_v<Deleter>);
        auto deletion_lambda = [d = std::move(d), ptr]() mutable noexcept { d(ptr); };
        shared_ptr_control_block *new_ctrl = nullptr;
        try {
            new_ctrl = new shared_ptr_control_block_impl<decltype(deletion_lambda)>(std::move(deletion_lambda));
        } catch (...) {
            deletion_lambda();
            throw;
        }
        m_ctrl = new_ctrl;
        m_ptr = ptr;
        maybe_enable_sharing_from_this(ptr);
    }

    template<class Y>
    shared_ptr(const shared_ptr<Y>& rhs, T* ptr) noexcept : m_ctrl(rhs.m_ctrl) {
        m_ptr = ptr;
        increment_use_count();
        // The aliasing constructor does NOT call maybe_enable_sharing_from_this!
    }

    shared_ptr(const shared_ptr& rhs) noexcept : shared_ptr(rhs, rhs.get()) {}
    template<class Y> shared_ptr(const shared_ptr<Y>& rhs) noexcept : shared_ptr(rhs, rhs.get()) {}

    shared_ptr(shared_ptr&& rhs) noexcept : shared_ptr(rhs, rhs.get()) { rhs.reset(); }
    template<class Y> shared_ptr(shared_ptr<Y>&& rhs) noexcept : shared_ptr(rhs, rhs.get()) { rhs.reset(); }

                      shared_ptr& operator=(const shared_ptr& rhs) noexcept    { shared_ptr<T>(rhs).swap(*this); return *this; }
    template<class Y> shared_ptr& operator=(const shared_ptr<Y>& rhs) noexcept { shared_ptr<T>(rhs).swap(*this); return *this; }
                      shared_ptr& operator=(shared_ptr&& rhs) noexcept         { shared_ptr<T>(std::move(rhs)).swap(*this); return *this; }
    template<class Y> shared_ptr& operator=(shared_ptr<Y>&& rhs) noexcept      { shared_ptr<T>(std::move(rhs)).swap(*this); return *this; }

    void swap(shared_ptr& rhs) noexcept {
        std::swap(m_ptr, rhs.m_ptr);
        std::swap(m_ctrl, rhs.m_ctrl);
    }

    ~shared_ptr() {
        decrement_use_count();
    }

                                     void reset() noexcept         { shared_ptr().swap(*this); }
    template<class Y>                void reset(Y *ptr)            { shared_ptr(ptr).swap(*this); }
    template<class Y, class Deleter> void reset(Y *ptr, Deleter d) { shared_ptr(ptr, std::move(d)).swap(*this); }

private:
    void increment_use_count() noexcept {
        // YOUR CODE GOES HERE
    }
    void decrement_use_count() noexcept {
        // YOUR CODE GOES HERE
    }

    template<class Y>
    void maybe_enable_sharing_from_this(Y *p) {
        maybe_enable_sharing_from_this_impl(p, p);
    }
    void maybe_enable_sharing_from_this_impl(...) { }
    template<class Y, class Z>
    void maybe_enable_sharing_from_this_impl(Y *, const std::enable_shared_from_this<Z> *p)
    {
        if (p != nullptr) {
            puts("shared_ptr's constructor has detected enable_shared_from_this!");
            puts(__PRETTY_FUNCTION__);
        }
    }

    shared_ptr_control_block *m_ctrl = nullptr;
};

template<class T> bool operator==(const shared_ptr<T>& a, decltype(nullptr)) noexcept { return a.get() == nullptr; }
template<class T> bool operator!=(const shared_ptr<T>& a, decltype(nullptr)) noexcept { return a.get() != nullptr; }

} // namespace my