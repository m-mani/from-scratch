#pragma once

#include "scratch/bits/smart-ptrs/default-delete.h"
#include "scratch/bits/smart-ptrs/forward-declarations.h"
#include "scratch/bits/smart-ptrs/shared-ptr-control-block.h"
#include "scratch/bits/smart-ptrs/smart-ptr-base.h"
#include "scratch/bits/type-traits/enable-if.h"
#include "scratch/bits/type-traits/is-foo.h"
#include "scratch/bits/type-traits/is-fooible.h"

#include <utility>

namespace scratch {

template<typename T>
class shared_ptr : public detail::smart_ptr_base<T>
{
    using detail::smart_ptr_base<T>::m_ptr;
public:
    using element_type = typename detail::smart_ptr_base<T>::element_type;

    constexpr shared_ptr() noexcept = default;
    constexpr shared_ptr(decltype(nullptr)) noexcept {}

    explicit shared_ptr(element_type *ptr) : shared_ptr(ptr, default_delete<T>{}) {}
    template<class Y, class Deleter> explicit shared_ptr(Y *ptr,            Deleter d) { reset(ptr, std::move(d)); }
    template<class Deleter>          explicit shared_ptr(decltype(nullptr), Deleter d) { reset(nullptr, std::move(d)); }

    template<class Y, bool_if_t<!is_array_v<T>> = true>
    explicit shared_ptr(Y *ptr) : shared_ptr(ptr, default_delete<Y>{}) {}


    template<class Y>
    shared_ptr(const shared_ptr<Y>& rhs, T* ptr) noexcept : m_ctrl(rhs.m_ctrl) {
        m_ptr = ptr;
        increment_use_count();
    }

    shared_ptr(const shared_ptr& rhs) noexcept : shared_ptr(rhs, rhs.get()) {}
    template<class Y> shared_ptr(const shared_ptr<Y>& rhs) noexcept : shared_ptr(rhs, rhs.get()) {}

    shared_ptr(shared_ptr&& rhs) noexcept {
        m_ptr = std::exchange(rhs.m_ptr, nullptr);
        m_ctrl = std::exchange(rhs.m_ctrl, nullptr);
    }

    template<class Y> shared_ptr(shared_ptr<Y>&& rhs) noexcept {
        m_ptr = std::exchange(rhs.m_ptr, nullptr);
        m_ctrl = std::exchange(rhs.m_ctrl, nullptr);
    }

                      shared_ptr& operator=(const shared_ptr& rhs) noexcept    { shared_ptr<T>(rhs).swap(*this); return *this; }
    template<class Y> shared_ptr& operator=(const shared_ptr<Y>& rhs) noexcept { shared_ptr<T>(rhs).swap(*this); return *this; }
                      shared_ptr& operator=(shared_ptr&& rhs) noexcept         { shared_ptr<T>(std::move(rhs)).swap(*this); return *this; }
    template<class Y> shared_ptr& operator=(shared_ptr<Y>&& rhs) noexcept      { shared_ptr<T>(std::move(rhs)).swap(*this); return *this; }

    template<class Y, class Deleter>
    shared_ptr(unique_ptr<Y, Deleter>&& rhs) {
        *this = std::move(rhs);
    }

    template<class Y, class Deleter>
    shared_ptr& operator=(unique_ptr<Y, Deleter>&& rhs) {
        shared_ptr<T>(rhs.get(), std::move(rhs.get_deleter())).swap(*this);
        rhs.release();
        return *this;
    }

    void swap(shared_ptr& rhs) noexcept {
        std::swap(m_ptr, rhs.m_ptr);
        std::swap(m_ctrl, rhs.m_ctrl);
    }

    ~shared_ptr() {
        decrement_use_count();
    }

    void reset() noexcept {
        decrement_use_count();
        m_ctrl = nullptr;
        m_ptr = nullptr;
    }
    template<class Y> void reset(Y *ptr) {
        this->reset(ptr, default_delete<Y>());
    }
    template<class Y, class Deleter> void reset(Y *ptr, Deleter d) {
        static_assert(is_nothrow_move_constructible_v<Deleter>);
        auto deletion_lambda = [d = std::move(d), ptr]() mutable noexcept { d(ptr); };
        detail::shared_ptr_control_block *new_ctrl = nullptr;
        try {
            new_ctrl = new detail::shared_ptr_control_block_impl<decltype(deletion_lambda)>(std::move(deletion_lambda));
        } catch (...) {
            deletion_lambda();
            throw;
        }
        this->reset();
        m_ctrl = new_ctrl;
        m_ptr = ptr;
    }

    long use_count() const noexcept {
        if (m_ctrl != nullptr) {
            return m_ctrl->m_use_count;
        } else {
            return 0;
        }
    }

private:
    void increment_use_count() noexcept {
        if (m_ctrl != nullptr) {
            ++m_ctrl->m_use_count;
        }
    }
    void decrement_use_count() noexcept {
        if (m_ctrl != nullptr) {
            int new_use_count = --m_ctrl->m_use_count;
            if (new_use_count == 0) {
                m_ctrl->delete_controlled_object();
                delete m_ctrl;
            }
        }
    }

    detail::shared_ptr_control_block *m_ctrl = nullptr;
};

template<class T> bool operator==(const shared_ptr<T>& a, decltype(nullptr)) noexcept { return a.get() == nullptr; }
template<class T> bool operator!=(const shared_ptr<T>& a, decltype(nullptr)) noexcept { return a.get() != nullptr; }
template<class T> bool operator==(decltype(nullptr), const shared_ptr<T>& a) noexcept { return a.get() == nullptr; }
template<class T> bool operator!=(decltype(nullptr), const shared_ptr<T>& a) noexcept { return a.get() != nullptr; }

template<class T, class U> bool operator==(const shared_ptr<T>& a, const shared_ptr<U>& b) noexcept { return a.get() == b.get(); }
template<class T, class U> bool operator!=(const shared_ptr<T>& a, const shared_ptr<U>& b) noexcept { return a.get() != b.get(); }

} // namespace scratch
