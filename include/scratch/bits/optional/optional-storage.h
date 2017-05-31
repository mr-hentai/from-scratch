#pragma once

#include "scratch/bits/optional/in-place.h"
#include "scratch/bits/type-traits/enable-if.h"
#include "scratch/bits/type-traits/is-fooible.h"

#include <initializer_list>
#include <new>
#include <utility>

namespace scratch::detail {

template<class T>
struct optional_storage {
    union {
        char m_dummy;
        T m_value;
    };
    bool m_has_value;

    constexpr optional_storage() noexcept : m_dummy(0), m_has_value(false) {}

    template<class... Args>
    constexpr optional_storage(in_place_t, Args&&... args)
        noexcept(is_nothrow_constructible_v<T, Args&&...>) :
        m_value(std::forward<Args>(args)...),
        m_has_value(true) {}

    template<class... Args, class U>
    constexpr optional_storage(in_place_t, std::initializer_list<U> il, Args&&... args)
        noexcept(is_nothrow_constructible_v<T, std::initializer_list<U>&, Args&&...>) :
        m_value(il, std::forward<Args>(args)...),
        m_has_value(true) {}

    constexpr bool storage_has_value() const noexcept { return m_has_value; }

    constexpr T& storage_value() noexcept { return m_value; }
    constexpr const T& storage_value() const noexcept { return m_value; }

    void storage_reset() noexcept {
        m_value.~T();
        m_has_value = false;
    }

    template<class... Args>
    void storage_emplace(Args&&... args) {
        ::new (&m_value) T(std::forward<Args>(args)...);
        m_has_value = true;
    }

    template<class U, class... Args>
    void storage_emplace(std::initializer_list<U> il, Args&&... args) {
        ::new (&m_value) T(il, std::forward<Args>(args)...);
        m_has_value = true;
    }
};

template<class T, class Enable = void>
struct optional_destructible : optional_storage<T>
{
    using optional_storage<T>::optional_storage;
    ~optional_destructible() {
        if (this->storage_has_value()) {
            this->storage_reset();
        }
    }
};

template<class T>
struct optional_destructible<T, enable_if_t<is_trivially_destructible_v<T>>> : optional_storage<T>
{
    using optional_storage<T>::optional_storage;
    ~optional_destructible() = default;
};

template<class T, class Enable = void>
struct optional_copyable : optional_destructible<T>
{
    using optional_destructible<T>::optional_destructible;
    optional_copyable() = default;

    template<bool_if_t<is_copy_constructible_v<T>> = true>
    optional_copyable(const optional_copyable& rhs)
        noexcept(is_nothrow_copy_constructible_v<T>)
    {
        if (rhs.storage_has_value()) {
            this->storage_emplace(rhs.storage_value());
        } else {
            // do nothing
        }
    }

    template<bool_if_t<is_move_constructible_v<T>> = true>
    optional_copyable(optional_copyable&& rhs)
        noexcept(is_nothrow_move_constructible_v<T>)
    {
        if (rhs.storage_has_value()) {
            this->storage_emplace(std::move(rhs.storage_value()));
        } else {
            // do nothing
        }
    }

    template<bool_if_t<is_copy_constructible_v<T> && is_copy_assignable_v<T>> = true>
    optional_copyable& operator=(const optional_copyable& rhs)
        noexcept(is_nothrow_copy_constructible_v<T> && is_nothrow_copy_assignable_v<T>)
    {
        if (this->storage_has_value()) {
            if (rhs.storage_has_value()) {
                this->storage_value() = rhs.storage_value();
            } else {
                this->storage_reset();
            }
        } else {
            if (rhs.storage_has_value()) {
                this->storage_emplace(rhs.storage_value());
            } else {
                // do nothing
            }
        }
        return *this;
    }

    template<bool_if_t<is_move_constructible_v<T> && is_move_assignable_v<T>> = true>
    optional_copyable& operator=(optional_copyable&& rhs)
        noexcept(is_nothrow_move_constructible_v<T> && is_nothrow_move_assignable_v<T>)
    {
        if (this->storage_has_value()) {
            if (rhs.storage_has_value()) {
                this->storage_value() = std::move(rhs.storage_value());
            } else {
                this->storage_reset();
            }
        } else {
            if (rhs.storage_has_value()) {
                this->storage_emplace(std::move(rhs.storage_value()));
            } else {
                // do nothing
            }
        }
        return *this;
    }
};

template<class T>
struct optional_copyable<T, enable_if_t<is_trivially_copyable_v<T>>> : optional_destructible<T>
{
    using optional_destructible<T>::optional_destructible;
    optional_copyable() = default;
    optional_copyable& operator=(const optional_copyable&) = default;
    optional_copyable(const optional_copyable&) = default;
};

} // namespace scratch::detail